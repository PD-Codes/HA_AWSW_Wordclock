"""Shared entity base for the AWSW WordClock."""

from __future__ import annotations

from collections.abc import Iterable
from typing import Any

from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers import entity_registry as er
from homeassistant.helpers.device_registry import (
    CONNECTION_NETWORK_MAC,
    DeviceInfo,
    format_mac,
)
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN, MANUFACTURER, MODEL
from .coordinator import WordClockCoordinator


def device_key(coordinator: WordClockCoordinator) -> str:
    """Return the stable identifier the entities are namespaced under.

    The config entry unique id is the device MAC, which survives removing and
    re-adding the integration. The entry id is only a fallback for a device
    that did not report a MAC.
    """
    entry = coordinator.config_entry
    return entry.unique_id or entry.entry_id


@callback
def async_remove_unsupported(
    hass: HomeAssistant,
    coordinator: WordClockCoordinator,
    domain: str,
    keys: Iterable[str],
) -> None:
    """Remove entities the connected firmware does not provide.

    The original AWSW firmware and the custom firmware expose slightly different
    features. Without this, switching firmware would leave orphaned entities behind.
    """
    registry = er.async_get(hass)
    prefix = device_key(coordinator)
    for key in keys:
        if entity_id := registry.async_get_entity_id(domain, DOMAIN, f"{prefix}_{key}"):
            registry.async_remove(entity_id)


class WordClockEntity(CoordinatorEntity[WordClockCoordinator]):
    """Base entity carrying the shared device information."""

    _attr_has_entity_name = True

    def __init__(self, coordinator: WordClockCoordinator, key: str) -> None:
        """Initialise the entity with a stable unique id."""
        super().__init__(coordinator)
        self._key = key
        self._attr_unique_id = f"{device_key(coordinator)}_{key}"

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
            identifiers={(DOMAIN, device_key(self.coordinator))},
            manufacturer=MANUFACTURER,
            model=MODEL,
            name=status.get("hostname") or f"WordClock ({host})",
            sw_version=status.get("version"),
            configuration_url=f"http://{host}/",
        )
        if mac := status.get("mac"):
            info["connections"] = {(CONNECTION_NETWORK_MAC, format_mac(mac))}
        return info

    @property
    def available(self) -> bool:
        """Return whether the last poll succeeded."""
        return super().available and bool(self.coordinator.data)
