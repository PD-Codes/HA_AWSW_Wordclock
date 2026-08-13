"""Shared entity base for the AWSW WordClock."""

from __future__ import annotations

from typing import Any

from homeassistant.helpers.device_registry import CONNECTION_NETWORK_MAC, DeviceInfo
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN, MANUFACTURER, MODEL
from .coordinator import WordClockCoordinator


class WordClockEntity(CoordinatorEntity[WordClockCoordinator]):
    """Base entity carrying the shared device information."""

    _attr_has_entity_name = True

    def __init__(self, coordinator: WordClockCoordinator, key: str) -> None:
        """Initialise the entity with a stable unique id."""
        super().__init__(coordinator)
        self._key = key
        entry_id = coordinator.config_entry.entry_id
        self._attr_unique_id = f"{entry_id}_{key}"

    @property
    def _status(self) -> dict[str, Any]:
        """Return the latest status payload."""
        return self.coordinator.data or {}

    @property
    def device_info(self) -> DeviceInfo:
        """Return the device this entity belongs to."""
        status = self._status
        host = self.coordinator.api.host
        info = DeviceInfo(
            identifiers={(DOMAIN, self.coordinator.config_entry.entry_id)},
            manufacturer=MANUFACTURER,
            model=MODEL,
            name=status.get("hostname") or f"WordClock ({host})",
            sw_version=status.get("version"),
            configuration_url=f"http://{host}/",
        )
        if mac := status.get("mac"):
            info["connections"] = {(CONNECTION_NETWORK_MAC, mac)}
        return info

    @property
    def available(self) -> bool:
        """Return whether the last poll succeeded."""
        return super().available and bool(self.coordinator.data)
