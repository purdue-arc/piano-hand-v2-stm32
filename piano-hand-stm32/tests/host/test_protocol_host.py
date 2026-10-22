"""
test_protocol_host.py — Unit tests for host/protocol.py command building and validation.
"""

import sys
from pathlib import Path
import pytest

# Ensure host directory is in sys.path
repo_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(repo_root))
sys.path.insert(0, str(repo_root / "host"))

from protocol import (  # type: ignore
    build_step,
    build_servo_on,
    build_servo_off,
    build_servo_on_multi,
    build_reset,
    build_stop,
    build_status,
    validate_finger,
    validate_step,
    validate_command_string,
)


def test_build_step_positive():
    """1. build_step(5) → '0:step:5'"""
    assert build_step(5) == "0:step:5"


def test_build_step_negative():
    """2. build_step(-3) → '0:step:-3'"""
    assert build_step(-3) == "0:step:-3"


def test_build_servo_on():
    """3. build_servo_on(3) → '0:servo_on:3'"""
    assert build_servo_on(3) == "0:servo_on:3"


def test_build_servo_off():
    """4. build_servo_off(1) → '0:servo_off:1'"""
    assert build_servo_off(1) == "0:servo_off:1"


def test_build_servo_on_multi():
    """5. build_servo_on_multi([1,3,5]) → '0:servo_on:1,3,5'"""
    assert build_servo_on_multi([1, 3, 5]) == "0:servo_on:1,3,5"


def test_validate_finger_zero():
    """6. validate_finger(0) → False"""
    assert validate_finger(0) is False


def test_validate_finger_six():
    """7. validate_finger(6) → False"""
    assert validate_finger(6) is False


def test_validate_finger_one():
    """8. validate_finger(1) → True"""
    assert validate_finger(1) is True


def test_validate_finger_five():
    """9. validate_finger(5) → True"""
    assert validate_finger(5) is True


def test_build_servo_on_invalid_zero():
    """10. build_servo_on(0) raises ValueError"""
    with pytest.raises(ValueError):
        build_servo_on(0)


def test_build_servo_on_invalid_six():
    """11. build_servo_on(6) raises ValueError"""
    with pytest.raises(ValueError):
        build_servo_on(6)


def test_validate_step_zero():
    """12. validate_step(0) → False (no-op step)"""
    assert validate_step(0) is False


def test_validate_step_five():
    """13. validate_step(5) → True"""
    assert validate_step(5) is True


def test_validate_command_string_valid_step():
    """14. validate_command_string('0:step:5') → (True, '')"""
    is_valid, err = validate_command_string("0:step:5")
    assert is_valid is True
    assert err == ""


def test_validate_command_string_invalid_step():
    """15. validate_command_string('0:step:abc') → (False, <some error>)"""
    is_valid, err = validate_command_string("0:step:abc")
    assert is_valid is False
    assert len(err) > 0


def test_validate_command_string_reset():
    """16. validate_command_string('RESET') → (True, '')"""
    is_valid, err = validate_command_string("RESET")
    assert is_valid is True
    assert err == ""


def test_build_standalone_commands():
    """Verify build_reset, build_stop, build_status."""
    assert build_reset() == "RESET"
    assert build_stop() == "STOP"
    assert build_status() == "STATUS"


def test_build_servo_on_multi_invalid():
    """Verify build_servo_on_multi with empty list or invalid finger raises ValueError."""
    with pytest.raises(ValueError):
        build_servo_on_multi([])
    with pytest.raises(ValueError):
        build_servo_on_multi([1, 7])
