"""
test_note_map.py — Unit tests for host/note_map.py translation stub.
"""

import sys
from pathlib import Path

repo_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(repo_root))
sys.path.insert(0, str(repo_root / "host"))

from note_map import (  # type: ignore
    parse_note_transition,
    note_to_key_index,
    note_transition_to_steps,
    NOTE_TO_KEY_INDEX,
)


def test_parse_note_transition_hyphen():
    from_n, to_n = parse_note_transition("F5-C4")
    assert from_n == "F5"
    assert to_n == "C4"


def test_parse_note_transition_arrow():
    from_n, to_n = parse_note_transition("F5>C4")
    assert from_n == "F5"
    assert to_n == "C4"


def test_parse_note_transition_whitespace_case():
    from_n, to_n = parse_note_transition("  g4 - a4  ")
    assert from_n == "G4"
    assert to_n == "A4"


def test_parse_note_transition_invalid():
    assert parse_note_transition("invalid") == (None, None)
    assert parse_note_transition("") == (None, None)
    assert parse_note_transition("C4") == (None, None)


def test_uncalibrated_returns_none():
    assert note_to_key_index("C4") is None
    assert note_transition_to_steps("C4", "D4") is None


def test_calibrated_lookup_simulation(monkeypatch):
    """Test simulated calibration mapping."""
    test_map = {"C4": 0, "D4": 1, "E4": 2, "F4": 3}
    monkeypatch.setattr("note_map.NOTE_TO_KEY_INDEX", test_map)

    assert note_to_key_index("C4") == 0
    assert note_to_key_index("d4") == 1
    assert note_transition_to_steps("C4", "E4") == 2
    assert note_transition_to_steps("E4", "C4") == -2
    assert note_transition_to_steps("C4", "B4") is None
