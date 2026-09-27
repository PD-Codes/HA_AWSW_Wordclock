"""Switch platform for the AWSW WordClock display options."""

from __future__ import annotations

from typing import Any

from homeassistant.components.switch import SwitchEntity, SwitchEntityDescription
from homeassistant.const import EntityCategory
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback

from .coordinator import WordClockConfigEntry, WordClockCoordinator
from .entity import WordClockEntity, async_remove_unsupported

SWITCHES: tuple[SwitchEntityDescription, ...] = (
    SwitchEntityDescription(
        key="nightMode",
        translation_key="night_mode",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="singleMinutes",
        translation_key="single_minutes",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="showItIs",
        translation_key="show_it_is",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="smoothTransition",
        translation_key="smooth_transition",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="digitalHourChime",
        translation_key="digital_hour_chime",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="randomDayColors",
        translation_key="random_day_colors",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="startupAnimation",
        translation_key="startup_animation",
        entity_category=EntityCategory.CONFIG,
    ),
    SwitchEntityDescription(
        key="showIp",
        translation_key="show_ip",
        entity_category=EntityCategory.CONFIG,
    ),
)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    """Set up the WordClock configuration switches."""
    coordinator = entry.runtime_data
    status = coordinator.data or {}
    # Only offer options the firmware actually reports (showItIs is custom firmware only)
    supported = [d for d in SWITCHES if d.key in status]
    async_remove_unsupported(
        hass, coordinator, "switch", (d.key for d in SWITCHES if d not in supported)
    )
    async_add_entities(
        WordClockSwitch(coordinator, description) for description in supported
    )


class WordClockSwitch(WordClockEntity, SwitchEntity):
    """A boolean display option exposed by /api/status."""

    entity_description: SwitchEntityDescription

    def __init__(
        self,
        coordinator: WordClockCoordinator,
        description: SwitchEntityDescription,
    ) -> None:
        """Initialise the switch."""
        super().__init__(coordinator, description.key)
        self.entity_description = description

    @property
    def is_on(self) -> bool:
        """Return the current value of the option."""
        return bool(self._status.get(self.entity_description.key))

    async def async_turn_on(self, **kwargs: Any) -> None:
        """Enable the option."""
        await self.coordinator.async_set(**{self.entity_description.key: True})

    async def async_turn_off(self, **kwargs: Any) -> None:
        """Disable the option."""
        await self.coordinator.async_set(**{self.entity_description.key: False})
