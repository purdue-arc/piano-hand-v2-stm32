"""
note_map.py — Note-to-stepper-key-index translation

STATUS: STUB — not calibrated

To use this module, you must:
1. Physically measure how many key positions (steps of STEPPER_DEG_PER_KEY degrees)
   the stepper must move from the home position to reach each piano key the hand uses.
2. Fill in NOTE_TO_KEY_INDEX with {note_name: key_index} where key_index is an integer
   relative to the home position (rightmost key = 0 for right hand, leftmost for left).
3. The sign convention must match STEPPER_DEG_PER_KEY sign in config.h.

Example (NOT calibrated, do not use):
    NOTE_TO_KEY_INDEX = {
        'C4': 0,
        'D4': 1,
        'E4': 2,
    }

Calibration procedure:
1. Connect to the robot via the controller.
2. Use CAL:SET_HOME to define home.
3. Use CAL:JOG:<degrees> to move to each key position.
4. Record the key index for each note.
5. Fill in NOTE_TO_KEY_INDEX below.
"""

from typing import Optional

# FILL THIS IN after physical calibration
NOTE_TO_KEY_INDEX: dict[str, int] = {}


def note_to_key_index(note: str) -> Optional[int]:
    """Returns the stepper key index for a note name, or None if not calibrated."""
    if not isinstance(note, str):
        return None
    return NOTE_TO_KEY_INDEX.get(note.strip().upper(), None)


def note_transition_to_steps(from_note: str, to_note: str) -> Optional[int]:
    """
    Returns the signed key step count to move from from_note to to_note.
    Returns None if either note is not in the calibration map.
    
    Steps = to_index - from_index.
    Positive = right, negative = left (matches STEPPER_DEG_PER_KEY sign).
    """
    from_idx = note_to_key_index(from_note)
    to_idx = note_to_key_index(to_note)
    if from_idx is None or to_idx is None:
        return None
    return to_idx - from_idx


def parse_note_transition(step_value: str) -> tuple[Optional[str], Optional[str]]:
    """
    Parse 'F5-C4' or 'F5>C4' into ('F5', 'C4').
    Returns (None, None) on failure.
    """
    if not isinstance(step_value, str):
        return None, None
    for sep in ("-", ">"):
        if sep in step_value:
            parts = step_value.split(sep, 1)
            if len(parts) == 2:
                p0 = parts[0].strip().upper()
                p1 = parts[1].strip().upper()
                if p0 and p1:
                    return p0, p1
    return None, None
