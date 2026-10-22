# Communication Protocol Specification — Piano Hand STM32

This document defines the wire framing, command syntax, BLE GATT characteristics, song file formats, and telemetry response packets for the Piano Hand system.

---

## 1. Transport Layer (Framing & Encoding)

Communication between the Host PC and the STM32 firmware is conducted over Bluetooth Low Energy (BLE) GATT characteristics, which bridge via UART to the STM32 microcontroller.

- **Encoding**: UTF-8 ASCII text.
- **Line Termination**: Each discrete packet must terminate with a newline character (`\n`, ASCII `0x0A`). Optional carriage returns (`\r`) are stripped by the parser.
- **Maximum Packet Length**: 64 bytes.
- **Baud Rate (Internal UART Bridge)**: 115,200 baud, 8 data bits, no parity, 1 stop bit (8N1).

---

## 2. Command Reference Table

Commands are colon-separated strings. Timestamps are required when streaming from song files, but immediate execution commands may omit the timestamp or pass `0`.

| Command Format | Arguments | Expected Response | Description & Constraints |
|---|---|---|---|
| `<ts>:step:<disp>` | `disp`: signed integer | `OK:STEP:<disp>` or `ERR:...` | Moves lateral carriage by `disp` key positions. Non-zero `int32` value. |
| `<ts>:servo_on:<fingers>` | `fingers`: comma-separated `1-5` | `OK:SERVO_ON:<fingers>` | Actuates specified finger servo(s) downward onto piano key(s). |
| `<ts>:servo_off:<fingers>` | `fingers`: comma-separated `1-5` | `OK:SERVO_OFF:<fingers>` | Releases specified finger servo(s) back to resting position. |
| `RESET` or `<ts>:RESET` | None | `OK:RESET` | Returns all fingers to resting position and moves carriage to home. |
| `STOP` or `<ts>:STOP` | None | `OK:STOP` | Emergency immediate halt: disables servo hold and halts stepper motion. |
| `STATUS` or `<ts>:STATUS` | None | `STATUS:<state>:<pos>:<fingers>` | Queries current robot operational state, carriage position, and finger mask. |

---

## 3. BLE GATT Characteristics Table

The system uses standard Nordic Semiconductor UART service-style 128-bit custom UUIDs.

| Attribute | UUID | Type | Description |
|---|---|---|---|
| **Service UUID** | `19b10000-e8f2-537e-4f6c-d104768a1214` | Service | Piano Hand BLE Service |
| **RX Characteristic** | `19b10001-e8f2-537e-4f6c-d104768a1214` | Write / Write Without Response | Commands sent from Host to Robot |
| **TX Characteristic** | `19b10002-e8f2-537e-4f6c-d104768a1214` | Notify / Read | Status, telemetry, and error notifications sent from Robot to Host |

---

## 4. Song File Format Specification

### 4.1 Canonical Format
The canonical song file is a plain-text file where each non-empty, non-comment line represents an event:

```text
# Lines starting with # are comments
# Timestamps are in SECONDS (float), monotonically non-decreasing
# Format: timestamp:command:value

0.0:step:0          # Initial reference position
1.0:step:-3         # Move stepper -3 key positions (left)
2.0:servo_on:3      # Press finger 3 (middle finger)
2.15:servo_off:3    # Release finger 3 after 150ms
2.5:servo_on:1,3,5  # Press fingers 1, 3, and 5 simultaneously
2.65:servo_off:1    # Release individual fingers
2.65:servo_off:3
2.65:servo_off:5
```

### 4.2 Timestamp Unit Detection Rules
The host parser (`host/song_parser.py`) applies the following hierarchy to determine timestamp units:
1. **CLI Flag Override**: If `--timestamp-unit s` or `--timestamp-unit ms` is passed, that unit is strictly enforced.
2. **Comment Header Directive**: If a comment matches `# timestamp_unit: ms` or `# timestamp_unit: s`, it establishes the unit for the entire file.
3. **Auto-Detection Heuristic**:
   - The parser inspects the maximum timestamp in the file.
   - If `max(timestamps) >= 1000.0`, the file is assumed to be in milliseconds (`ms`) and timestamps are automatically divided by `1000.0` into seconds.
   - Otherwise, the timestamps are treated as seconds (`s`).

### 4.3 Note Transitions (Uncalibrated)
Song files may contain note-to-note transitions using `-` or `>` delimiters:
```text
1.0:step:F5-C4
2.0:step:C4>G4
```
> [!IMPORTANT]
> Note transitions (`step:F5-C4`) are **NOT** directly sent over BLE to the robot firmware. The host controller resolves them using `host/note_map.py`. If a note transition has not been calibrated, the event is skipped with a warning log.

---

## 5. Status Response Format

When polled with `STATUS` or when transitioning states, the firmware transmits a telemetry packet over TX notification:

```text
STATUS:<STATE>:<STEPPER_POS>:<ACTIVE_FINGERS>
```

- `<STATE>`: String enumeration (`IDLE`, `PLAYING`, `HOMING`, `CALIBRATING`, `ERROR`).
- `<STEPPER_POS>`: Signed integer indicating current key position relative to home.
- `<ACTIVE_FINGERS>`: Hexadecimal bitmask (e.g., `0x00` = none, `0x05` = fingers 1 and 3 active).

Example:
```text
STATUS:IDLE:0:0x00\n
STATUS:PLAYING:-3:0x04\n
```

---

## 6. Error Response Format

If an incoming command fails validation, or a hardware condition triggers a fault:

```text
ERR:<ERROR_CODE>:<DESCRIPTION>\n
```

### Standard Error Codes:
| Error Code | Meaning |
|---|---|
| `ERR:PARSE_FAIL` | Ill-formed packet, missing colons, or unrecognized verb. |
| `ERR:INVALID_FINGER` | Finger index outside 1–5 range. |
| `ERR:INVALID_STEP` | Step value zero, non-integer, or outside int32 boundaries. |
| `ERR:BUFFER_OVERFLOW` | Command queue full; host is streaming faster than execution rate. |
| `ERR:MOTOR_FAULT` | Stepper driver reported fault or limit switch struck unexpectedly. |
| `ERR:I2C_TIMEOUT` | PCA9685 did not acknowledge bus transaction. |

---

## 7. Calibration Command Reference

During physical commissioning, the host uses calibration commands to measure key positions and home offsets:

| Calibration Command | Description |
|---|---|
| `CAL:SET_HOME` | Defines current carriage position as software key index `0`. |
| `CAL:JOG:<degrees>` | Steps carriage by exact degrees (float) to align with a physical key. |
| `CAL:QUERY_POS` | Queries current accumulator degrees and key index. |
| `CAL:SERVO_TRIM:<finger>:<offset_us>` | Fine-tunes microsecond pulse width for individual finger heights. |
