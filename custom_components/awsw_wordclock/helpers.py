"""Conversion helpers shared by the AWSW WordClock platforms."""

from __future__ import annotations

MAX_HA_BRIGHTNESS = 255


def hex_to_rgb(value: str | None) -> tuple[int, int, int]:
    """Convert a ``#RRGGBB`` string to an RGB tuple, falling back to black."""
    if not isinstance(value, str):
        return (0, 0, 0)
    raw = value.lstrip("#")
    if len(raw) != 6:
        return (0, 0, 0)
    try:
        return (int(raw[0:2], 16), int(raw[2:4], 16), int(raw[4:6], 16))
    except ValueError:
        return (0, 0, 0)


def rgb_to_hex(rgb: tuple[int, int, int]) -> str:
    """Convert an RGB tuple to the ``#RRGGBB`` string the firmware expects."""
    r, g, b = (max(0, min(255, int(channel))) for channel in rgb)
    return f"#{r:02X}{g:02X}{b:02X}"


def split_color_brightness(
    rgb: tuple[int, int, int],
) -> tuple[tuple[int, int, int], int]:
    """Split a stored colour into full-intensity hue plus a brightness value.

    The extra word API stores only a raw RGB triplet, so brightness has to be
    encoded in the colour itself. The brightest channel is treated as the
    brightness and the colour is normalised back to full scale, which makes the
    round trip stable.
    """
    peak = max(rgb)
    if peak == 0:
        return (255, 255, 255), 0
    scale = MAX_HA_BRIGHTNESS / peak
    normalised = tuple(
        min(255, round(channel * scale)) for channel in rgb
    )
    return normalised, peak  # type: ignore[return-value]


def apply_brightness(
    rgb: tuple[int, int, int], brightness: int
) -> tuple[int, int, int]:
    """Scale a full-intensity colour down to the requested brightness."""
    factor = max(0, min(MAX_HA_BRIGHTNESS, brightness)) / MAX_HA_BRIGHTNESS
    scaled = tuple(round(channel * factor) for channel in rgb)
    return scaled  # type: ignore[return-value]


def device_to_ha_brightness(value: int, limit: int) -> int:
    """Scale a firmware brightness (0..limit) to Home Assistant's 0..255."""
    if limit <= 0:
        return 0
    return max(0, min(MAX_HA_BRIGHTNESS, round(value / limit * MAX_HA_BRIGHTNESS)))


def ha_to_device_brightness(value: int, limit: int) -> int:
    """Scale a Home Assistant brightness (0..255) to the firmware's 0..limit."""
    if limit <= 0:
        return 0
    return max(0, min(limit, round(value / MAX_HA_BRIGHTNESS * limit)))


def strip_word_prefix(name: str) -> str:
    """Return an extra word name without the firmware's ``"3: "`` numbering."""
    _, separator, remainder = name.partition(":")
    return remainder.strip() if separator else name.strip()
