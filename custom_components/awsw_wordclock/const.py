"""Constants for the AWSW WordClock integration."""

from __future__ import annotations

from typing import Final

from homeassistant.const import Platform

DOMAIN: Final = "awsw_wordclock"
MANUFACTURER: Final = "AWSW"
MODEL: Final = "WordClock 16x16"

PLATFORMS: Final[list[Platform]] = [
    Platform.BINARY_SENSOR,
    Platform.BUTTON,
    Platform.LIGHT,
    Platform.NOTIFY,
    Platform.SENSOR,
    Platform.SWITCH,
]

CONF_HOST: Final = "host"

# Legacy option/data keys kept only for config entry migration.
LEGACY_CONF_IP_ADDRESS: Final = "ip_address"
LEGACY_CONF_LANGUAGE: Final = "language"

DEFAULT_SCAN_INTERVAL_SECONDS: Final = 30
# The custom firmware answers /api/status in a few milliseconds, so poll it faster.
FAST_SCAN_INTERVAL_SECONDS: Final = 5
REQUEST_TIMEOUT_SECONDS: Final = 10

# The firmware caps every brightness slider at this value; it is read from
# /api/status at runtime and this is only the fallback.
DEFAULT_INTENSITY_LIMIT: Final = 50

# --- /api/status keys -------------------------------------------------------

KEY_EXTRA_WORDS: Final = "extraWords"
KEY_FIRMWARE: Final = "firmware"
KEY_INTENSITY_LIMIT: Final = "intensityLimit"
KEY_MAC: Final = "mac"

# Colour keys, all exchanged as "#RRGGBB".
KEY_TIME_COLOR: Final = "timeColor"
KEY_TIME_COLOR_NIGHT: Final = "timeColorNight"
KEY_BACK_COLOR: Final = "backColor"
KEY_BACK_COLOR_NIGHT: Final = "backColorNight"

# Brightness keys, integers in the range 0..intensityLimit.
KEY_TIME_BRIGHTNESS_DAY: Final = "timeBrightnessDay"
KEY_TIME_BRIGHTNESS_NIGHT: Final = "timeBrightnessNight"
KEY_BACK_BRIGHTNESS_DAY: Final = "backBrightnessDay"
KEY_BACK_BRIGHTNESS_NIGHT: Final = "backBrightnessNight"
KEY_TICKER_COLOR: Final = "tickerColor"

# --- /api/action commands ---------------------------------------------------

ACTION_RESTART: Final = "restart"
ACTION_TEST: Final = "test"
ACTION_DIGITAL_TIME_TEST: Final = "digitalTimeTest"
ACTION_WORD_RESET: Final = "wordReset"
ACTION_RESET_EXTRA_WORDS: Final = "resetExtraWords"
ACTION_UPDATE_CHECK: Final = "updateCheck"
ACTION_WIFI_OPTIMIZE: Final = "wifiOptimize"


# Value of KEY_FIRMWARE reported by the custom WordClock firmware (V6+). The original
# AWSW firmware does not report this key.
FIRMWARE_CUSTOM: Final = "WordClock Custom"


def is_custom_firmware(status: dict) -> bool:
    """Return True if the clock runs the custom WordClock firmware."""
    return status.get(KEY_FIRMWARE) == FIRMWARE_CUSTOM


def extra_word_active_key(word_id: int) -> str:
    """Return the /api/set key toggling an extra word."""
    return f"ew{word_id}"


def extra_word_color_key(word_id: int) -> str:
    """Return the /api/set key setting the colour of an extra word."""
    return f"ewColor{word_id}"
