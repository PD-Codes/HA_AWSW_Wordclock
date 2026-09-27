"""Binary sensors for the AWSW WordClock."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

from homeassistant.components.binary_sensor import (
    BinarySensorDeviceClass,
    BinarySensorEntity,
    BinarySensorEntityDescription,
)
from homeassistant.const import EntityCategory
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback

from .coordinator import WordClockConfigEntry, WordClockCoordinator
from .entity import WordClockEntity, async_remove_unsupported


@dataclass(frozen=True, kw_only=True)
class WordClockBinarySensorDescription(BinarySensorEntityDescription):
    """Binary sensor description with a value extractor."""

    value_fn: Callable[[dict[str, Any]], bool] = lambda status: False
    supported_fn: Callable[[dict[str, Any]], bool] = lambda status: True


BINARY_SENSORS: tuple[WordClockBinarySensorDescription, ...] = (
    WordClockBinarySensorDescription(
        key="ntp_ok",
        translation_key="ntp_ok",
        device_class=BinarySensorDeviceClass.CONNECTIVITY,
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: bool(status.get("ntpOk")),
    ),
    WordClockBinarySensorDescription(
        key="online_mode",
        translation_key="online_mode",
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: bool(status.get("onlineMode")),
    ),
    WordClockBinarySensorDescription(
        key="update_available",
        translation_key="update_available",
        device_class=BinarySensorDeviceClass.UPDATE,
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: bool(status.get("updateButtonActive")),
        supported_fn=lambda status: "updateButtonActive" in status,
    ),
)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    """Set up the WordClock binary sensors."""
    coordinator = entry.runtime_data
    status = coordinator.data or {}
    supported = [d for d in BINARY_SENSORS if d.supported_fn(status)]
    async_remove_unsupported(
        hass,
        coordinator,
        "binary_sensor",
        (d.key for d in BINARY_SENSORS if d not in supported),
    )
    async_add_entities(
        WordClockBinarySensor(coordinator, description) for description in supported
    )


class WordClockBinarySensor(WordClockEntity, BinarySensorEntity):
    """A boolean state derived from /api/status."""

    entity_description: WordClockBinarySensorDescription

    def __init__(
        self,
        coordinator: WordClockCoordinator,
        description: WordClockBinarySensorDescription,
    ) -> None:
        """Initialise the binary sensor."""
        super().__init__(coordinator, description.key)
        self.entity_description = description

    @property
    def is_on(self) -> bool:
        """Return the current state."""
        return self.entity_description.value_fn(self._status)
