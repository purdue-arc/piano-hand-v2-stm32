"""
song_parser.py — Song file parser for the Piano Hand robot.

Parses song files in canonical format (timestamp:command:value) or legacy variants
into a list of SongEvent objects, converting timestamps to seconds and validating
finger indices and monotonic timing.
"""

from dataclasses import dataclass, field
import logging
import os
import re
from typing import Optional, Union

logger = logging.getLogger(__name__)

# Valid finger bounds
FINGER_MIN = 1
FINGER_MAX = 5


@dataclass
class SongEvent:
    timestamp_s: float
    raw_command: str  # 'step', 'servo_on', 'servo_off', 'reset', etc.
    raw_value: str  # raw value string (may be note name, finger list, or integer)
    is_note_transition: bool  # True if value is 'NOTE-NOTE' or 'NOTE>NOTE' format
    fingers: list[int] = field(default_factory=list)  # finger list for servo commands
    step_keys: Optional[int] = None  # None if is_note_transition


def detect_timestamp_unit(timestamps: list[float]) -> str:
    """
    Returns 's' or 'ms' based on heuristic.
    
    If the maximum timestamp >= 1000.0, assume timestamps are in milliseconds (ms);
    otherwise assume seconds (s).
    """
    if not timestamps:
        return "s"
    if max(timestamps) >= 1000.0:
        return "ms"
    return "s"


def _check_note_transition(val: str) -> bool:
    """Check if value represents a note-to-note transition separated by '-' or '>'."""
    for sep in ("-", ">"):
        if sep in val:
            parts = val.split(sep, 1)
            if len(parts) == 2 and parts[0].strip() and parts[1].strip():
                return True
    return False


def parse_song_file(
    path: Union[str, os.PathLike],
    timestamp_unit: str = "auto",
) -> list[SongEvent]:
    """
    Parse a song file into a list of SongEvent objects.
    
    Args:
        path: Path to the song file.
        timestamp_unit: 'auto', 's', or 'ms'.
            If 'auto', looks for '# timestamp_unit: <unit>' header in the file comments.
            If not found in comments, auto-detects based on timestamp magnitudes.
            
    Returns:
        List of SongEvent instances with timestamps normalized to seconds.
        
    Raises:
        ValueError: If timestamps are not monotonically non-decreasing, or if
                    an invalid finger index is encountered.
        FileNotFoundError: If the specified song file does not exist.
    """
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    header_unit: Optional[str] = None
    raw_entries: list[tuple[float, str, str, str]] = []  # (raw_ts, cmd, val, raw_line)
    all_raw_timestamps: list[float] = []

    # First pass: check for comment directives and collect lines
    for line_num, raw_line in enumerate(lines, start=1):
        stripped = raw_line.strip()
        if not stripped:
            continue

        # Check comment lines
        if stripped.startswith("#"):
            unit_match = re.search(r"timestamp_unit:\s*([a-zA-Z]+)", stripped)
            if unit_match and header_unit is None:
                header_unit = unit_match.group(1).lower()
            continue

        # Strip inline comments
        content = raw_line.split("#", 1)[0].strip()
        if not content:
            continue

        # Parse colon-separated parts
        parts = [p.strip() for p in content.split(":")]
        if len(parts) < 2:
            logger.warning("Line %d: Skipping unparseable line: '%s'", line_num, raw_line.strip())
            continue

        try:
            raw_ts = float(parts[0])
        except ValueError:
            logger.warning("Line %d: Invalid timestamp '%s': '%s'", line_num, parts[0], raw_line.strip())
            continue

        cmd = parts[1].lower()
        val = parts[2] if len(parts) > 2 else ""

        all_raw_timestamps.append(raw_ts)
        raw_entries.append((raw_ts, cmd, val, raw_line.strip()))

    # Determine final timestamp unit
    effective_unit: str
    if timestamp_unit in ("s", "ms"):
        effective_unit = timestamp_unit
    elif header_unit in ("s", "ms"):
        effective_unit = header_unit
    else:
        effective_unit = detect_timestamp_unit(all_raw_timestamps)

    scale_factor = 0.001 if effective_unit == "ms" else 1.0

    events: list[SongEvent] = []
    last_timestamp_s: Optional[float] = None

    for raw_ts, cmd, val, line_text in raw_entries:
        ts_s = raw_ts * scale_factor

        # Verify monotonic non-decreasing timestamp
        if last_timestamp_s is not None and ts_s < last_timestamp_s:
            raise ValueError(
                f"Non-monotonic timestamp detected: {ts_s:.4f}s < previous {last_timestamp_s:.4f}s in line: '{line_text}'"
            )
        last_timestamp_s = ts_s

        # Process command and value
        if cmd == "step":
            if _check_note_transition(val):
                events.append(
                    SongEvent(
                        timestamp_s=ts_s,
                        raw_command=cmd,
                        raw_value=val,
                        is_note_transition=True,
                        fingers=[],
                        step_keys=None,
                    )
                )
            else:
                try:
                    step_val = int(val)
                except ValueError:
                    logger.warning("Skipping step with non-integer value '%s': '%s'", val, line_text)
                    continue

                events.append(
                    SongEvent(
                        timestamp_s=ts_s,
                        raw_command=cmd,
                        raw_value=val,
                        is_note_transition=False,
                        fingers=[],
                        step_keys=step_val,
                    )
                )

        elif cmd in ("servo_on", "servo_off"):
            if not val:
                logger.warning("Servo command missing finger value: '%s'", line_text)
                continue

            finger_tokens = val.split(",")
            parsed_fingers: list[int] = []
            for tok in finger_tokens:
                t = tok.strip()
                try:
                    f_idx = int(t)
                except ValueError:
                    raise ValueError(f"Invalid non-integer finger index '{t}' in line: '{line_text}'")
                
                if f_idx < FINGER_MIN or f_idx > FINGER_MAX:
                    raise ValueError(
                        f"Finger index {f_idx} out of range [{FINGER_MIN}, {FINGER_MAX}] in line: '{line_text}'"
                    )
                parsed_fingers.append(f_idx)

            events.append(
                SongEvent(
                    timestamp_s=ts_s,
                    raw_command=cmd,
                    raw_value=val,
                    is_note_transition=False,
                    fingers=parsed_fingers,
                    step_keys=None,
                )
            )

        else:
            # Other commands (e.g. reset, stop, status)
            events.append(
                SongEvent(
                    timestamp_s=ts_s,
                    raw_command=cmd,
                    raw_value=val,
                    is_note_transition=False,
                    fingers=[],
                    step_keys=None,
                )
            )

    return events
