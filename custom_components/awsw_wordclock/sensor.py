"""Diagnostic sensors for the AWSW WordClock."""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

from homeassistant.components.sensor import (
    SensorDeviceClass,
    SensorEntity,
    SensorEntityDescription,
    SensorStateClass,
)
from homeassistant.const import EntityCategory, SIGNAL_STRENGTH_DECIBELS_MILLIWATT
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback

from .coordinator import WordClockConfigEntry, WordClockCoordinator
from .entity import WordClockEntity


@dataclass(frozen=True, kw_only=True)
class WordClockSensorDescription(SensorEntityDescription):
    """Sensor description with a value extractor."""

    value_fn: Callable[[dict[str, Any]], Any] = lambda status: None


SENSORS: tuple[WordClockSensorDescription, ...] = (
    WordClockSensorDescription(
        key="rssi",
        translation_key="rssi",
        device_class=SensorDeviceClass.SIGNAL_STRENGTH,
        native_unit_of_measurement=SIGNAL_STRENGTH_DECIBELS_MILLIWATT,
        state_class=SensorStateClass.MEASUREMENT,
        entity_category=EntityCategory.DIAGNOSTIC,
        entity_registry_enabled_default=False,
        value_fn=lambda status: status.get("rssi"),
    ),
    WordClockSensorDescription(
        key="ssid",
        translation_key="ssid",
        entity_category=EntityCategory.DIAGNOSTIC,
        entity_registry_enabled_default=False,
        value_fn=lambda status: status.get("ssid"),
    ),
    WordClockSensorDescription(
        key="ip",
        translation_key="ip",
        entity_category=EntityCategory.DIAGNOSTIC,
        entity_registry_enabled_default=False,
        value_fn=lambda status: status.get("ip"),
    ),
    WordClockSensorDescription(
        key="version",
        translation_key="version",
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: status.get("version"),
    ),
    WordClockSensorDescription(
        key="available_version",
        translation_key="available_version",
        entity_category=EntityCategory.DIAGNOSTIC,
        entity_registry_enabled_default=False,
        value_fn=lambda status: status.get("availableVersion"),
    ),
    WordClockSensorDescription(
        key="device_time",
        translation_key="device_time",
        value_fn=lambda status: status.get("time"),
    ),
    WordClockSensorDescription(
        key="night_status",
        translation_key="night_status",
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: status.get("nightStatus"),
    ),
    WordClockSensorDescription(
        key="ntp_status",
        translation_key="ntp_status",
        entity_category=EntityCategory.DIAGNOSTIC,
        entity_registry_enabled_default=False,
        value_fn=lambda status: status.get("ntpStatus"),
    ),
    WordClockSensorDescription(
        key="time_server",
        translation_key="time_server",
        entity_category=EntityCategory.DIAGNOSTIC,
        entity_registry_enabled_default=False,
        value_fn=lambda status: status.get("timeServer"),
    ),
    WordClockSensorDescription(
        key="language_name",
        translation_key="language_name",
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: status.get("languageName"),
    ),
    WordClockSensorDescription(
        key="intensity",
        translation_key="intensity",
        state_class=SensorStateClass.MEASUREMENT,
        entity_category=EntityCategory.DIAGNOSTIC,
        value_fn=lambda status: status.get("intensity"),
    ),
)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    """Set up the WordClock diagnostic sensors."""
    coordinator = entry.runtime_data
    async_add_entities(
        WordClockSensor(coordinator, description) for description in SENSORS
    )


class WordClockSensor(WordClockEntity, SensorEntity):
    """A single read-only value from /api/status."""

    entity_description: WordClockSensorDescription

    def __init__(
        self,
        coordinator: WordClockCoordinator,
        description: WordClockSensorDescription,
    ) -> None:
        """Initialise the sensor."""
        super().__init__(coordinator, description.key)
        self.entity_description = description

    @property
    def native_value(self) -> Any:
        """Return the current value."""
        return self.entity_description.value_fn(self._status)
