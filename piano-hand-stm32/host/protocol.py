"""
protocol.py — Command building and validation for Piano Hand robot.

Defines the protocol wire format, validation rules, and command builders.
This module has no BLE or I/O dependencies.

Wire format:
    timestamp:command:value  OR  command:value  OR  command
Examples:
    0:step:5
    1.5:servo_on:1,3,5
    2.0:servo_off:3
    RESET
    STOP
    STATUS
"""

from typing import Union

FINGER_MIN: int = 1
FINGER_MAX: int = 5
INT32_MIN: int = -2147483648
INT32_MAX: int = 2147483647

VALID_COMMANDS = {"step", "servo_on", "servo_off", "reset", "stop", "status"}
STANDALONE_COMMANDS = {"RESET", "STOP", "STATUS"}


def _format_timestamp(ts: float) -> str:
    """Format timestamp concisely: integer if whole number, else float."""
    if ts == int(ts):
        return str(int(ts))
    return f"{ts:g}"


def validate_step(key_displacement: int) -> bool:
    """
    Returns True if displacement is within int32 range and non-zero.
    Zero is considered a no-op and thus invalid for a step command.
    """
    if not isinstance(key_displacement, int) or isinstance(key_displacement, bool):
        return False
    if key_displacement == 0:
        return False
    return INT32_MIN <= key_displacement <= INT32_MAX


def validate_finger(finger: int) -> bool:
    """Returns True if finger index is within valid range [1, 5]."""
    if not isinstance(finger, int) or isinstance(finger, bool):
        return False
    return FINGER_MIN <= finger <= FINGER_MAX


def build_step(key_displacement: int, timestamp: float = 0.0) -> str:
    """
    Build a step command string.
    
    Args:
        key_displacement: Signed integer number of key positions to move.
        timestamp: Command timestamp in seconds (default 0.0 for immediate).
        
    Returns:
        Formatted command string, e.g. '0:step:5' or '0:step:-3'.
        
    Raises:
        ValueError: If key_displacement is invalid or zero.
    """
    if not validate_step(key_displacement):
        raise ValueError(
            f"Invalid key displacement {key_displacement}. Must be non-zero int32."
        )
    return f"{_format_timestamp(timestamp)}:step:{key_displacement}"


def build_servo_on(finger: int, timestamp: float = 0.0) -> str:
    """
    Build a servo_on command string for a single finger.
    
    Args:
        finger: Finger index (1-5).
        timestamp: Command timestamp in seconds (default 0.0).
        
    Returns:
        Formatted command string, e.g. '0:servo_on:3'.
        
    Raises:
        ValueError: If finger index is not between 1 and 5.
    """
    if not validate_finger(finger):
        raise ValueError(
            f"Invalid finger {finger}. Must be between {FINGER_MIN} and {FINGER_MAX}."
        )
    return f"{_format_timestamp(timestamp)}:servo_on:{finger}"


def build_servo_off(finger: int, timestamp: float = 0.0) -> str:
    """
    Build a servo_off command string for a single finger.
    
    Args:
        finger: Finger index (1-5).
        timestamp: Command timestamp in seconds (default 0.0).
        
    Returns:
        Formatted command string, e.g. '0:servo_off:1'.
        
    Raises:
        ValueError: If finger index is not between 1 and 5.
    """
    if not validate_finger(finger):
        raise ValueError(
            f"Invalid finger {finger}. Must be between {FINGER_MIN} and {FINGER_MAX}."
        )
    return f"{_format_timestamp(timestamp)}:servo_off:{finger}"


def build_servo_on_multi(fingers: list[int], timestamp: float = 0.0) -> str:
    """
    Build a servo_on command string for multiple fingers pressed simultaneously.
    
    Args:
        fingers: List of finger indices (each 1-5).
        timestamp: Command timestamp in seconds (default 0.0).
        
    Returns:
        Formatted command string, e.g. '0:servo_on:1,3,5'.
        
    Raises:
        ValueError: If fingers list is empty or any finger index is invalid.
    """
    if not fingers:
        raise ValueError("Finger list cannot be empty.")
    for f in fingers:
        if not validate_finger(f):
            raise ValueError(
                f"Invalid finger {f} in list {fingers}. Must be between {FINGER_MIN} and {FINGER_MAX}."
            )
    finger_str = ",".join(str(f) for f in fingers)
    return f"{_format_timestamp(timestamp)}:servo_on:{finger_str}"


def build_reset() -> str:
    """Build a RESET command string."""
    return "RESET"


def build_stop() -> str:
    """Build a STOP command string."""
    return "STOP"


def build_status() -> str:
    """Build a STATUS command string."""
    return "STATUS"


def validate_command_string(cmd: str) -> tuple[bool, str]:
    """
    Parses and validates a command string.
    
    Supported formats:
        - Standalone: 'RESET', 'STOP', 'STATUS', or calibration 'CAL:...'
        - 2-part: 'command:value'
        - 3-part: 'timestamp:command:value'
        
    Returns:
        (is_valid, error_message). If valid, error_message is ''.
    """
    if not isinstance(cmd, str):
        return False, "Command must be a string."
    
    s = cmd.strip()
    if not s:
        return False, "Command cannot be empty."

    # Check standalone uppercase commands
    if s.upper() in STANDALONE_COMMANDS:
        return True, ""

    # Check calibration commands (e.g. CAL:SET_HOME, CAL:JOG:10)
    if s.startswith("CAL:"):
        return True, ""

    parts = s.split(":")
    
    if len(parts) == 1:
        if parts[0].upper() in STANDALONE_COMMANDS:
            return True, ""
        return False, f"Unknown single command '{parts[0]}'."
    
    elif len(parts) == 2:
        # Could be 'timestamp:STANDALONE' e.g. '0:RESET' or 'command:value'
        try:
            ts = float(parts[0])
            if ts < 0:
                return False, f"Timestamp cannot be negative: {ts}"
            if parts[1].upper() in STANDALONE_COMMANDS:
                return True, ""
        except ValueError:
            pass

        command = parts[0].lower()
        value = parts[1]
    
    elif len(parts) == 3:
        # 'timestamp:command:value'
        try:
            ts = float(parts[0])
            if ts < 0:
                return False, f"Timestamp cannot be negative: {ts}"
        except ValueError:
            return False, f"Invalid timestamp format '{parts[0]}'."
        
        command = parts[1].lower()
        value = parts[2]
        
    else:
        return False, f"Command has too many colon-separated parts ({len(parts)})."

    if command not in VALID_COMMANDS:
        return False, f"Unknown command verb '{command}'."

    if command == "step":
        try:
            disp = int(value)
        except ValueError:
            return False, f"Step displacement '{value}' is not a valid integer."
        if not validate_step(disp):
            return False, f"Step displacement {disp} is invalid (must be non-zero int32)."
        return True, ""

    elif command in ("servo_on", "servo_off"):
        finger_parts = value.split(",")
        for fp in finger_parts:
            fp_clean = fp.strip()
            try:
                f_idx = int(fp_clean)
            except ValueError:
                return False, f"Finger value '{fp_clean}' is not an integer."
            if not validate_finger(f_idx):
                return False, f"Finger {f_idx} out of range ({FINGER_MIN}-{FINGER_MAX})."
        return True, ""

    elif command in ("reset", "stop", "status"):
        return True, ""

    return False, f"Unhandled command format: '{cmd}'."
