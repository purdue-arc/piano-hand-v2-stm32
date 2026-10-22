"""
test_song_parser.py — Unit tests for host/song_parser.py.
"""

import sys
from pathlib import Path
import pytest

repo_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(repo_root))
sys.path.insert(0, str(repo_root / "host"))

from song_parser import SongEvent, parse_song_file, detect_timestamp_unit  # type: ignore


def test_parse_canonical_format(tmp_path):
    """1. Parse canonical format (seconds, integer step) → correct SongEvent list."""
    f = tmp_path / "canonical.txt"
    f.write_text(
        "1.0:step:-3\n"
        "2.0:servo_on:3\n"
        "2.15:servo_off:3\n",
        encoding="utf-8"
    )
    events = parse_song_file(f, timestamp_unit="s")
    assert len(events) == 3
    assert events[0] == SongEvent(1.0, "step", "-3", False, [], -3)
    assert events[1] == SongEvent(2.0, "servo_on", "3", False, [3], None)
    assert events[2] == SongEvent(2.15, "servo_off", "3", False, [3], None)


def test_parse_servo_multi_finger(tmp_path):
    """2. Parse servo with multiple fingers: servo_on:1,3,5 → fingers=[1,3,5]."""
    f = tmp_path / "multi_finger.txt"
    f.write_text("2.0:servo_on:1,3,5\n", encoding="utf-8")
    events = parse_song_file(f)
    assert len(events) == 1
    assert events[0].raw_command == "servo_on"
    assert events[0].fingers == [1, 3, 5]


def test_parse_note_transition_hyphen(tmp_path):
    """3. Parse note transition: step:F5-C4 → is_note_transition=True, step_keys=None."""
    f = tmp_path / "note_trans_hyphen.txt"
    f.write_text("0.5:step:F5-C4\n", encoding="utf-8")
    events = parse_song_file(f)
    assert len(events) == 1
    assert events[0].is_note_transition is True
    assert events[0].step_keys is None
    assert events[0].raw_value == "F5-C4"


def test_parse_note_transition_arrow(tmp_path):
    """4. Parse '>' separator: step:F5>C4 → is_note_transition=True."""
    f = tmp_path / "note_trans_arrow.txt"
    f.write_text("0.5:step:F5>C4\n", encoding="utf-8")
    events = parse_song_file(f)
    assert len(events) == 1
    assert events[0].is_note_transition is True
    assert events[0].step_keys is None
    assert events[0].raw_value == "F5>C4"


def test_timestamp_auto_detect_ms(tmp_path):
    """5. Timestamp unit auto-detection: file with 1000.0:step:3 → detected as ms, converted to 1.0s."""
    f = tmp_path / "legacy_ms.txt"
    f.write_text("1000.0:step:3\n", encoding="utf-8")
    events = parse_song_file(f, timestamp_unit="auto")
    assert len(events) == 1
    assert events[0].timestamp_s == pytest.approx(1.0)
    assert events[0].step_keys == 3


def test_timestamp_auto_detect_seconds(tmp_path):
    """6. Timestamp unit auto-detection: file with 1.0:step:3 → detected as seconds."""
    f = tmp_path / "seconds.txt"
    f.write_text("1.0:step:3\n", encoding="utf-8")
    events = parse_song_file(f, timestamp_unit="auto")
    assert len(events) == 1
    assert events[0].timestamp_s == pytest.approx(1.0)
    assert events[0].step_keys == 3


def test_non_monotonic_timestamps_raise_value_error(tmp_path):
    """7. Non-monotonic timestamps raise ValueError."""
    f = tmp_path / "non_monotonic.txt"
    f.write_text(
        "2.0:step:1\n"
        "1.5:step:2\n",
        encoding="utf-8"
    )
    with pytest.raises(ValueError, match="Non-monotonic"):
        parse_song_file(f)


def test_comments_are_skipped(tmp_path):
    """8. Comment lines are skipped."""
    f = tmp_path / "comments.txt"
    f.write_text(
        "# Header comment\n"
        "1.0:step:2  # inline comment\n"
        "# Another comment\n",
        encoding="utf-8"
    )
    events = parse_song_file(f)
    assert len(events) == 1
    assert events[0].step_keys == 2


def test_empty_lines_are_skipped(tmp_path):
    """9. Empty lines are skipped."""
    f = tmp_path / "empty_lines.txt"
    f.write_text(
        "\n\n"
        "1.0:step:1\n"
        "\n"
        "2.0:step:2\n"
        "\n\n",
        encoding="utf-8"
    )
    events = parse_song_file(f)
    assert len(events) == 2


def test_invalid_finger_raises_value_error(tmp_path):
    """10. Invalid finger index raises ValueError."""
    f_zero = tmp_path / "finger_zero.txt"
    f_zero.write_text("1.0:servo_on:0\n", encoding="utf-8")
    with pytest.raises(ValueError):
        parse_song_file(f_zero)

    f_six = tmp_path / "finger_six.txt"
    f_six.write_text("1.0:servo_on:6\n", encoding="utf-8")
    with pytest.raises(ValueError):
        parse_song_file(f_six)


def test_mixed_step_commands(tmp_path):
    """11. Mixed step commands (integer + note transition) in same file."""
    f = tmp_path / "mixed.txt"
    f.write_text(
        "0.0:step:0\n"
        "1.0:step:F5-C4\n"
        "2.0:step:2\n"
        "3.0:step:C4>G4\n",
        encoding="utf-8"
    )
    events = parse_song_file(f)
    assert len(events) == 4
    assert events[0].is_note_transition is False
    assert events[0].step_keys == 0
    assert events[1].is_note_transition is True
    assert events[1].step_keys is None
    assert events[2].is_note_transition is False
    assert events[2].step_keys == 2
    assert events[3].is_note_transition is True
    assert events[3].step_keys is None


def test_comment_header_unit_override(tmp_path):
    """Test that # timestamp_unit: ms sets the unit when auto-detecting."""
    f = tmp_path / "header_unit.txt"
    f.write_text(
        "# timestamp_unit: ms\n"
        "500.0:step:1\n",
        encoding="utf-8"
    )
    events = parse_song_file(f, timestamp_unit="auto")
    assert len(events) == 1
    assert events[0].timestamp_s == pytest.approx(0.5)
