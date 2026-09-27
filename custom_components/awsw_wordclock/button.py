"""Buttons exposing the AWSW WordClock maintenance actions."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

from homeassistant.components.button import (
    ButtonDeviceClass,
    ButtonEntity,
    ButtonEntityDescription,
)
from homeassistant.const import EntityCategory
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import HomeAssistantError
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback
from homeassistant.util import dt as dt_util

from .api import WordClockError
from .const import (
    ACTION_DIGITAL_TIME_TEST,
    ACTION_RESET_EXTRA_WORDS,
    ACTION_RESTART,
    ACTION_TEST,
    ACTION_UPDATE_CHECK,
    ACTION_WIFI_OPTIMIZE,
    ACTION_WORD_RESET,
    is_custom_firmware,
)
from .coordinator import WordClockConfigEntry, WordClockCoordinator
from .entity import WordClockEntity, async_remove_unsupported

SYNC_TIME_KEY = "sync_time"


@dataclass(frozen=True, kw_only=True)
class WordClockButtonDescription(ButtonEntityDescription):
    """Button description carrying the firmware command to run."""

    command: str
    supported_fn: Callable[[dict[str, Any]], bool] = lambda status: True


BUTTONS: tuple[WordClockButtonDescription, ...] = (
    WordClockButtonDescription(
        key="restart",
        translation_key="restart",
        device_class=ButtonDeviceClass.RESTART,
        entity_category=EntityCategory.CONFIG,
        command=ACTION_RESTART,
    ),
    WordClockButtonDescription(
        key="test",
        translation_key="test",
        entity_category=EntityCategory.CONFIG,
        command=ACTION_TEST,
    ),
    WordClockButtonDescription(
        key="digital_time_test",
        translation_key="digital_time_test",
        entity_category=EntityCategory.CONFIG,
        command=ACTION_DIGITAL_TIME_TEST,
    ),
    WordClockButtonDescription(
        key="word_reset",
        translation_key="word_reset",
        entity_category=EntityCategory.CONFIG,
        command=ACTION_WORD_RESET,
    ),
    WordClockButtonDescription(
        key="reset_extra_words",
        translation_key="reset_extra_words",
        entity_category=EntityCategory.CONFIG,
        command=ACTION_RESET_EXTRA_WORDS,
    ),
    WordClockButtonDescription(
        key="update_check",
        translation_key="update_check",
        entity_category=EntityCategory.CONFIG,
        command=ACTION_UPDATE_CHECK,
        supported_fn=lambda status: not is_custom_firmware(status),
    ),
    WordClockButtonDescription(
        key="wifi_optimize",
        translation_key="wifi_optimize",
        entity_category=EntityCategory.CONFIG,
        command=ACTION_WIFI_OPTIMIZE,
        supported_fn=lambda status: not is_custom_firmware(status),
    ),
)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    """Set up the WordClock buttons."""
    coordinator = entry.runtime_data
    status = coordinator.data or {}
    supported = [d for d in BUTTONS if d.supported_fn(status)]
    async_remove_unsupported(
        hass, coordinator, "button", (d.key for d in BUTTONS if d not in supported)
    )
    entities: list[ButtonEntity] = [
        WordClockActionButton(coordinator, description) for description in supported
    ]
    entities.append(WordClockSyncTimeButton(coordinator))
    async_add_entities(entities)


class WordClockActionButton(WordClockEntity, ButtonEntity):
    """Runs a single /api/action command."""

    entity_description: WordClockButtonDescription

    def __init__(
        self,
        coordinator: WordClockCoordinator,
        description: WordClockButtonDescription,
    ) -> None:
        """Initialise the button."""
        super().__init__(coordinator, description.key)
        self.entity_description = description

    async def async_press(self) -> None:
        """Run the command."""
        # A restart takes the device offline for a while, so polling right
        # afterwards would only mark every entity unavailable.
        refresh = self.entity_description.command != ACTION_RESTART
        await self.coordinator.async_action(
            self.entity_description.command, refresh=refresh
        )


class WordClockSyncTimeButton(WordClockEntity, ButtonEntity):
    """Pushes Home Assistant's current time to the clock."""

    _attr_translation_key = SYNC_TIME_KEY
    _attr_entity_category = EntityCategory.CONFIG

    def __init__(self, coordinator: WordClockCoordinator) -> None:
        """Initialise the button."""
        super().__init__(coordinator, SYNC_TIME_KEY)

    async def async_press(self) -> None:
        """Send the current timestamp to the device."""
        # Seconds resolution without microseconds keeps the string short enough
        # for the parser on the ESP32.
        timestamp = dt_util.now().replace(microsecond=0).isoformat(timespec="seconds")
        try:
            await self.coordinator.api.async_set_time(timestamp)
        except WordClockError as err:
            raise HomeAssistantError(
                f"Could not set the time on the WordClock: {err}"
            ) from err
        await self.coordinator.async_request_refresh()
