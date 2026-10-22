"""
controller.py — Host-side BLE controller for the Piano Hand robot.

Connects to the STM32 / ESP32 piano hand robot via BLE (Bleak), streams
timestamped commands from a song file or provides an interactive manual CLI,
handles status notifications, non-blocking servo scheduling, and automatic reset.

Usage:
    python host/controller.py --hand left --file examples/hot_cross_buns.txt
    python host/controller.py --hand right --interactive
"""

import argparse
import asyncio
import logging
import os
import sys
from typing import Optional

try:
    from bleak import BleakClient, BleakScanner
    from bleak.backends.characteristic import BleakGATTCharacteristic
except ImportError:
    BleakClient = None  # type: ignore
    BleakScanner = None  # type: ignore

try:
    from host.protocol import (
        build_reset,
        build_servo_off,
        build_servo_on,
        build_servo_on_multi,
        build_status,
        build_step,
        build_stop,
        validate_command_string,
    )
    from host.song_parser import SongEvent, parse_song_file
    from host.note_map import parse_note_transition, note_transition_to_steps
except ImportError:
    from protocol import (  # type: ignore
        build_reset,
        build_servo_off,
        build_servo_on,
        build_servo_on_multi,
        build_status,
        build_step,
        build_stop,
        validate_command_string,
    )
    from song_parser import SongEvent, parse_song_file  # type: ignore
    from note_map import parse_note_transition, note_transition_to_steps  # type: ignore

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)
logger = logging.getLogger("PianoController")

BLE_CONFIGS = {
    "left": {
        "device_name": "ESP32-Piano",  # legacy name; will change for STM32
        "rx_uuid": "19b10001-e8f2-537e-4f6c-d104768a1214",
        "tx_uuid": "19b10002-e8f2-537e-4f6c-d104768a1214",
    },
    "right": {
        "device_name": "ESP32-Piano",
        "rx_uuid": "19b10001-e8f2-537e-4f6c-d104768a1214",
        "tx_uuid": "19b10002-e8f2-537e-4f6c-d104768a1214",
    },
}


def notification_handler(sender: BleakGATTCharacteristic, data: bytearray) -> None:
    """Callback triggered whenever the robot publishes status notifications on TX characteristic."""
    try:
        msg = data.decode("utf-8", errors="replace").strip()
        logger.info("[ROBOT NOTIFY] %s", msg)
        print(f"\n<< [Robot Status] {msg}")
    except Exception as e:
        logger.error("Error decoding notification: %s", e)


async def schedule_servo_off(
    client: BleakClient,
    rx_uuid: str,
    fingers: list[int],
    delay: float = 0.15,
) -> None:
    """
    Non-blocking servo release task.
    Waits for `delay` seconds, then sends servo_off for each specified finger.
    Does NOT block the main playback loop.
    """
    try:
        await asyncio.sleep(delay)
        if client.is_connected:
            for finger in fingers:
                cmd = build_servo_off(finger)
                logger.debug("Scheduled release: %s", cmd)
                await client.write_gatt_char(rx_uuid, f"{cmd}\n".encode("utf-8"))
    except Exception as e:
        logger.warning("Error during scheduled servo_off for finger(s) %s: %s", fingers, e)


async def send_command(client: BleakClient, rx_uuid: str, cmd: str) -> bool:
    """Validate and write a single command string to the robot RX characteristic."""
    is_valid, err = validate_command_string(cmd)
    if not is_valid:
        logger.error("Validation failed for command '%s': %s", cmd, err)
        return False
    
    wire_bytes = f"{cmd.strip()}\n".encode("utf-8")
    logger.debug("Sending BLE payload: %s", wire_bytes)
    try:
        await client.write_gatt_char(rx_uuid, wire_bytes)
        return True
    except Exception as e:
        logger.error("BLE write failed for '%s': %s", cmd, e)
        return False


async def connect_with_retry(
    device_name: str,
    max_retries: int = 3,
    retry_delay: float = 2.0,
    disconnect_event: Optional[asyncio.Event] = None,
) -> BleakClient:
    """
    Scan and connect to the robot with a retry loop.
    
    Args:
        device_name: BLE advertised local name or identifier.
        max_retries: Number of connection attempts before failing.
        retry_delay: Delay in seconds between retries.
        disconnect_event: Optional asyncio Event signaled if connection drops.
        
    Returns:
        Connected BleakClient instance.
        
    Raises:
        ConnectionError: If connection fails after all attempts.
    """
    if BleakScanner is None:
        raise RuntimeError("Bleak library is not installed. Run 'pip install bleak'.")

    def on_disconnect(c: BleakClient):
        logger.warning("BLE connection lost to %s", c.address)
        if disconnect_event:
            disconnect_event.set()

    for attempt in range(1, max_retries + 1):
        logger.info("Attempt %d/%d: Scanning for BLE device '%s'...", attempt, max_retries, device_name)
        try:
            device = await BleakScanner.find_device_by_name(device_name, timeout=5.0)
            if device is None:
                logger.warning("Device '%s' not found in scan.", device_name)
            else:
                logger.info("Found device %s (%s). Connecting...", device.name, device.address)
                client = BleakClient(device, disconnected_callback=on_disconnect)
                await client.connect()
                if client.is_connected:
                    logger.info("Successfully connected to %s", device_name)
                    return client
        except Exception as e:
            logger.warning("Connection attempt %d failed: %s", attempt, e)

        if attempt < max_retries:
            await asyncio.sleep(retry_delay)

    raise ConnectionError(f"Failed to connect to '{device_name}' after {max_retries} attempts.")


async def play_song(
    client: BleakClient,
    rx_uuid: str,
    events: list[SongEvent],
    auto_release: bool = False,
    hold_time: float = 0.15,
) -> None:
    """
    Plays a parsed song without timing drift.
    
    Args:
        client: Connected BleakClient.
        rx_uuid: RX characteristic UUID to write to.
        events: List of SongEvent instances with timestamps in seconds.
        auto_release: If True, schedules servo_off after `hold_time` for each servo_on.
        hold_time: Duration in seconds to hold finger before releasing.
    """
    if not events:
        logger.warning("No events to play.")
        return

    logger.info("Starting song playback (%d events)...", len(events))
    loop = asyncio.get_running_loop()
    start_time = loop.time()

    for i, ev in enumerate(events):
        now = loop.time()
        target_time = start_time + ev.timestamp_s
        delay = target_time - now
        if delay > 0:
            await asyncio.sleep(delay)

        logger.info(
            "[%03d/%03d @ %.2fs] Command: %s, Value: %s",
            i + 1, len(events), ev.timestamp_s, ev.raw_command, ev.raw_value
        )

        if ev.raw_command == "step":
            if ev.is_note_transition:
                from_note, to_note = parse_note_transition(ev.raw_value)
                if from_note is None or to_note is None:
                    logger.warning("Malformed note transition '%s'. Skipping.", ev.raw_value)
                    continue
                steps = note_transition_to_steps(from_note, to_note)
                if steps is None:
                    logger.warning(
                        "Note transition '%s' -> '%s' is not calibrated in note_map.py. Skipping event.",
                        from_note, to_note
                    )
                    continue
                if steps == 0:
                    continue
                cmd = build_step(steps)
            else:
                if ev.step_keys is None or ev.step_keys == 0:
                    continue
                cmd = build_step(ev.step_keys)
            
            await send_command(client, rx_uuid, cmd)

        elif ev.raw_command == "servo_on":
            if len(ev.fingers) == 1:
                cmd = build_servo_on(ev.fingers[0])
            else:
                cmd = build_servo_on_multi(ev.fingers)
            
            await send_command(client, rx_uuid, cmd)

            if auto_release:
                asyncio.create_task(schedule_servo_off(client, rx_uuid, ev.fingers, hold_time))

        elif ev.raw_command == "servo_off":
            for f in ev.fingers:
                cmd = build_servo_off(f)
                await send_command(client, rx_uuid, cmd)

        elif ev.raw_command == "reset":
            cmd = build_reset()
            await send_command(client, rx_uuid, cmd)

        elif ev.raw_command == "stop":
            cmd = build_stop()
            await send_command(client, rx_uuid, cmd)

        elif ev.raw_command == "status":
            cmd = build_status()
            await send_command(client, rx_uuid, cmd)

        else:
            logger.warning("Unrecognized event raw_command '%s'. Skipping.", ev.raw_command)

    logger.info("Song stream complete. Sending RESET and waiting 3.0s...")
    await send_command(client, rx_uuid, build_reset())
    await asyncio.sleep(3.0)
    logger.info("Playback and homing reset finished successfully.")


async def interactive_mode(client: BleakClient, rx_uuid: str) -> None:
    """Prompt user for manual commands over BLE."""
    print("\n--- Piano Hand Interactive Mode ---")
    print("Commands supported:")
    print("  step:<displacement>     e.g., step:3 or step:-2")
    print("  servo_on:<finger(s)>    e.g., servo_on:1 or servo_on:1,3,5")
    print("  servo_off:<finger>      e.g., servo_off:1")
    print("  RESET, STOP, STATUS")
    print("  Type 'q', 'quit', or 'exit' to disconnect.\n")

    loop = asyncio.get_running_loop()

    while client.is_connected:
        try:
            user_input = await loop.run_in_executor(None, input, "piano> ")
        except (EOFError, KeyboardInterrupt):
            break

        line = user_input.strip()
        if not line:
            continue
        if line.lower() in ("q", "quit", "exit"):
            break

        cmd_to_send = line
        # If user typed 'step:5' without timestamp prefix, format via builder or validate directly
        if ":" in line and not line.startswith("CAL:"):
            parts = line.split(":")
            if len(parts) == 2 and not parts[0].isdigit():
                # E.g. 'step:5' -> '0:step:5'
                cmd_to_send = f"0:{line}"

        is_valid, err = validate_command_string(cmd_to_send)
        if not is_valid:
            # Check if raw uppercase standalone
            if line.upper() in ("RESET", "STOP", "STATUS"):
                cmd_to_send = line.upper()
                is_valid = True
            else:
                print(f"[Error] Invalid command: {err}")
                continue

        await send_command(client, rx_uuid, cmd_to_send)


async def main() -> None:
    parser = argparse.ArgumentParser(description="Piano Hand BLE Controller")
    parser.add_argument("--hand", choices=["left", "right"], default="left", help="Hand side (left/right)")
    parser.add_argument("--file", type=str, default=None, help="Path to song file")
    parser.add_argument("--device-name", type=str, default=None, help="Override BLE device advertised name")
    parser.add_argument("--rx-uuid", type=str, default=None, help="Override BLE RX UUID")
    parser.add_argument("--tx-uuid", type=str, default=None, help="Override BLE TX UUID")
    parser.add_argument("--timestamp-unit", choices=["s", "ms", "auto"], default="auto", help="Timestamp unit override")
    parser.add_argument("--max-retries", type=int, default=3, help="Max BLE connection attempts")
    parser.add_argument("--hold-time", type=float, default=0.15, help="Servo hold time in seconds for auto-release")
    parser.add_argument("--interactive", action="store_true", help="Launch interactive manual CLI")

    args = parser.parse_args()

    cfg = BLE_CONFIGS.get(args.hand, BLE_CONFIGS["left"]).copy()
    device_name = args.device_name or cfg["device_name"]
    rx_uuid = args.rx_uuid or cfg["rx_uuid"]
    tx_uuid = args.tx_uuid or cfg["tx_uuid"]

    logger.info("Target configuration: Hand=%s, Device='%s', RX=%s, TX=%s", args.hand, device_name, rx_uuid, tx_uuid)

    disconnect_event = asyncio.Event()

    try:
        client = await connect_with_retry(
            device_name=device_name,
            max_retries=args.max_retries,
            disconnect_event=disconnect_event,
        )
    except ConnectionError as e:
        logger.error("Could not establish connection: %s", e)
        sys.exit(1)

    try:
        # Start notifications on TX characteristic
        logger.info("Subscribing to TX notifications on %s...", tx_uuid)
        await client.start_notify(tx_uuid, notification_handler)

        if args.file:
            if not os.path.exists(args.file):
                logger.error("Song file '%s' not found.", args.file)
                sys.exit(1)

            events = parse_song_file(args.file, timestamp_unit=args.timestamp_unit)
            has_explicit_off = any(ev.raw_command == "servo_off" for ev in events)
            auto_release = not has_explicit_off

            logger.info(
                "Parsed %d events from '%s'. Auto-release scheduled servo_off: %s",
                len(events), args.file, auto_release
            )
            await play_song(
                client=client,
                rx_uuid=rx_uuid,
                events=events,
                auto_release=auto_release,
                hold_time=args.hold_time,
            )
        else:
            await interactive_mode(client, rx_uuid)

    except KeyboardInterrupt:
        logger.info("Playback interrupted by user. Sending emergency STOP...")
        try:
            await send_command(client, rx_uuid, build_stop())
        except Exception:
            pass
    finally:
        if client.is_connected:
            logger.info("Disconnecting from %s...", device_name)
            try:
                await client.stop_notify(tx_uuid)
            except Exception:
                pass
            await client.disconnect()
            logger.info("Disconnected cleanly.")


if __name__ == "__main__":
    asyncio.run(main())
