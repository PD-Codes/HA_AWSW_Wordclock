"""Light platform for the AWSW WordClock."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from homeassistant.components.light import (
    ATTR_BRIGHTNESS,
    ATTR_RGB_COLOR,
    ColorMode,
    LightEntity,
)
from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers import entity_registry as er
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback

from .const import (
    KEY_BACK_BRIGHTNESS_DAY,
    KEY_BACK_BRIGHTNESS_NIGHT,
    KEY_BACK_COLOR,
    KEY_BACK_COLOR_NIGHT,
    KEY_TIME_BRIGHTNESS_DAY,
    KEY_TIME_BRIGHTNESS_NIGHT,
    KEY_TIME_COLOR,
    KEY_TIME_COLOR_NIGHT,
    extra_word_active_key,
    extra_word_color_key,
)
from .coordinator import WordClockConfigEntry, WordClockCoordinator
from .entity import WordClockEntity, device_key
from .helpers import (
    apply_brightness,
    device_to_ha_brightness,
    ha_to_device_brightness,
    hex_to_rgb,
    rgb_to_hex,
    split_color_brightness,
    strip_word_prefix,
)

EXTRA_WORD_PREFIX = "extra_word_"


@dataclass(frozen=True, kw_only=True)
class WordClockPanelLightDescription:
    """Describes one of the clock's colour/brightness pairs."""

    key: str
    translation_key: str
    color_key: str
    brightness_key: str


PANEL_LIGHTS: tuple[WordClockPanelLightDescription, ...] = (
    WordClockPanelLightDescription(
        key="time_day",
        translation_key="time_day",
        color_key=KEY_TIME_COLOR,
        brightness_key=KEY_TIME_BRIGHTNESS_DAY,
    ),
    WordClockPanelLightDescription(
        key="time_night",
        translation_key="time_night",
        color_key=KEY_TIME_COLOR_NIGHT,
        brightness_key=KEY_TIME_BRIGHTNESS_NIGHT,
    ),
    WordClockPanelLightDescription(
        key="background_day",
        translation_key="background_day",
        color_key=KEY_BACK_COLOR,
        brightness_key=KEY_BACK_BRIGHTNESS_DAY,
    ),
    WordClockPanelLightDescription(
        key="background_night",
        translation_key="background_night",
        color_key=KEY_BACK_COLOR_NIGHT,
        brightness_key=KEY_BACK_BRIGHTNESS_NIGHT,
    ),
)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    """Set up the WordClock lights."""
    coordinator = entry.runtime_data

    _cleanup_stale_extra_words(hass, entry, coordinator)

    entities: list[LightEntity] = [
        WordClockPanelLight(coordinator, description) for description in PANEL_LIGHTS
    ]
    entities.extend(
        WordClockExtraWordLight(coordinator, word["id"])
        for word in coordinator.extra_words
    )
    async_add_entities(entities)


@callback
def _cleanup_stale_extra_words(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    coordinator: WordClockCoordinator,
) -> None:
    """Remove extra word entities the device no longer reports.

    A language change on the clock can shrink the number of extra words, which
    would otherwise leave dead entities behind. If the device reported no words
    at all the payload is treated as untrustworthy and nothing is removed.
    """
    words = coordinator.extra_words
    if not words:
        return

    registry = er.async_get(hass)
    prefix = f"{device_key(coordinator)}_{EXTRA_WORD_PREFIX}"
    valid = {f"{prefix}{word['id']}" for word in words}
    for registry_entry in er.async_entries_for_config_entry(registry, entry.entry_id):
        unique_id = registry_entry.unique_id
        if unique_id.startswith(prefix) and unique_id not in valid:
            registry.async_remove(registry_entry.entity_id)


class WordClockPanelLight(WordClockEntity, LightEntity):
    """A colour plus brightness pair of the clock face."""

    _attr_color_mode = ColorMode.RGB
    _attr_supported_color_modes = {ColorMode.RGB}

    def __init__(
        self,
        coordinator: WordClockCoordinator,
        description: WordClockPanelLightDescription,
    ) -> None:
        """Initialise the panel light."""
        super().__init__(coordinator, description.key)
        self._description = description
        self._attr_translation_key = description.translation_key
        # Remembers the level to restore when the panel is switched back on.
        self._last_on_brightness: int | None = None

    @property
    def is_on(self) -> bool:
        """Return true if the firmware brightness is above zero."""
        return self._device_brightness > 0

    @property
    def brightness(self) -> int:
        """Return the brightness scaled to Home Assistant's range."""
        return device_to_ha_brightness(
            self._device_brightness, self.coordinator.intensity_limit
        )

    @property
    def rgb_color(self) -> tuple[int, int, int]:
        """Return the configured colour."""
        return hex_to_rgb(self._status.get(self._description.color_key))

    @property
    def _device_brightness(self) -> int:
        value = self._status.get(self._description.brightness_key)
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            return 0
        return int(value)

    async def async_turn_on(self, **kwargs: Any) -> None:
        """Turn the panel on, optionally changing colour and brightness."""
        payload: dict[str, Any] = {}
        limit = self.coordinator.intensity_limit

        if (rgb := kwargs.get(ATTR_RGB_COLOR)) is not None:
            payload[self._description.color_key] = rgb_to_hex(rgb)

        if (brightness := kwargs.get(ATTR_BRIGHTNESS)) is not None:
            # Never round a positive request down to zero, which would read
            # back as "off" while Home Assistant believes the light is on.
            payload[self._description.brightness_key] = max(
                1, ha_to_device_brightness(brightness, limit)
            )
        elif not self.is_on:
            payload[self._description.brightness_key] = (
                self._last_on_brightness or limit
            )

        if payload:
            await self.coordinator.async_set(**payload)

    async def async_turn_off(self, **kwargs: Any) -> None:
        """Turn the panel off by setting its brightness to zero."""
        if (current := self._device_brightness) > 0:
            self._last_on_brightness = current
        await self.coordinator.async_set(**{self._description.brightness_key: 0})


class WordClockExtraWordLight(WordClockEntity, LightEntity):
    """One extra word, controllable as an RGB light.

    The firmware stores only a raw colour per word, so brightness is expressed
    by scaling that colour.
    """

    _attr_color_mode = ColorMode.RGB
    _attr_supported_color_modes = {ColorMode.RGB}

    def __init__(self, coordinator: WordClockCoordinator, word_id: int) -> None:
        """Initialise the extra word light."""
        super().__init__(coordinator, f"{EXTRA_WORD_PREFIX}{word_id}")
        self._word_id = word_id

    @property
    def name(self) -> str:
        """Return the word as printed on the clock face."""
        word = self.coordinator.extra_word(self._word_id)
        if word and (raw := word.get("name")):
            return strip_word_prefix(str(raw))
        return f"Extra word {self._word_id}"

    @property
    def is_on(self) -> bool:
        """Return whether the word is currently lit."""
        word = self.coordinator.extra_word(self._word_id)
        return bool(word and word.get("active")) and any(self._stored_rgb)

    @property
    def rgb_color(self) -> tuple[int, int, int]:
        """Return the word colour normalised to full intensity."""
        color, _ = split_color_brightness(self._stored_rgb)
        return color

    @property
    def brightness(self) -> int:
        """Return the brightness encoded in the stored colour."""
        _, brightness = split_color_brightness(self._stored_rgb)
        return brightness

    @property
    def _stored_rgb(self) -> tuple[int, int, int]:
        word = self.coordinator.extra_word(self._word_id)
        return hex_to_rgb(word.get("color") if word else None)

    async def async_turn_on(self, **kwargs: Any) -> None:
        """Light the word, optionally with a new colour or brightness."""
        current_color, current_brightness = split_color_brightness(self._stored_rgb)

        if (requested := kwargs.get(ATTR_RGB_COLOR)) is not None:
            # Normalise first: a caller may pass an already dimmed triplet, and
            # scaling that again would make the colour decay on every call.
            color, implied_brightness = split_color_brightness(requested)
        else:
            color, implied_brightness = current_color, None

        brightness = kwargs.get(ATTR_BRIGHTNESS)
        if brightness is None:
            brightness = implied_brightness or current_brightness or 255

        payload = {
            extra_word_color_key(self._word_id): rgb_to_hex(
                apply_brightness(color, max(1, brightness))
            ),
            extra_word_active_key(self._word_id): 1,
        }
        await self.coordinator.async_set(**payload)

    async def async_turn_off(self, **kwargs: Any) -> None:
        """Switch the word off without losing its colour."""
        await self.coordinator.async_set(**{extra_word_active_key(self._word_id): 0})
