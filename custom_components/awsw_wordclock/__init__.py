"""The AWSW WordClock integration."""

from __future__ import annotations

import logging
import re

from homeassistant.core import HomeAssistant
from homeassistant.helpers import device_registry as dr
from homeassistant.helpers import entity_registry as er
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.device_registry import format_mac

from .api import WordClockApi
from .const import (
    CONF_HOST,
    DOMAIN,
    KEY_MAC,
    LEGACY_CONF_IP_ADDRESS,
    LEGACY_CONF_LANGUAGE,
    PLATFORMS,
)
from .coordinator import WordClockConfigEntry, WordClockCoordinator

_LOGGER = logging.getLogger(__name__)

# Unique id pattern of the switches created by version 1.x of this integration.
LEGACY_SWITCH_UNIQUE_ID = re.compile(r"_word_\d+$")


async def async_setup_entry(hass: HomeAssistant, entry: WordClockConfigEntry) -> bool:
    """Set up AWSW WordClock from a config entry."""
    api = WordClockApi(entry.data[CONF_HOST], async_get_clientsession(hass))
    coordinator = WordClockCoordinator(hass, entry, api)
    await coordinator.async_config_entry_first_refresh()

    # Adopt the MAC as the entry unique id the first time we see it, so that a
    # device that changed its IP can still be recognised. Entity unique ids are
    # derived from it, so this has to happen before the platforms are set up.
    if entry.unique_id is None and (mac := coordinator.data.get(KEY_MAC)):
        hass.config_entries.async_update_entry(entry, unique_id=format_mac(mac))

    _async_rekey_registry(hass, entry)

    entry.runtime_data = coordinator
    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    return True


async def async_unload_entry(hass: HomeAssistant, entry: WordClockConfigEntry) -> bool:
    """Unload a config entry."""
    return await hass.config_entries.async_unload_platforms(entry, PLATFORMS)


async def async_migrate_entry(hass: HomeAssistant, entry: WordClockConfigEntry) -> bool:
    """Migrate an old config entry to the current layout.

    Version 1 stored the device address under ``ip_address`` and a manually
    chosen ``language``. Both are obsolete: the address key was renamed and the
    firmware now reports the extra words, including their names, itself.
    """
    if entry.version > 2:
        # Downgrades are not supported.
        return False

    if entry.version == 1:
        data = {**entry.data}
        options = {**entry.options}

        host = data.pop(LEGACY_CONF_IP_ADDRESS, None) or data.get(CONF_HOST)
        if not host:
            _LOGGER.error("Cannot migrate WordClock entry without an address")
            return False
        data[CONF_HOST] = host
        data.pop(LEGACY_CONF_LANGUAGE, None)
        options.pop(LEGACY_CONF_LANGUAGE, None)

        _async_remove_legacy_entities(hass, entry)

        hass.config_entries.async_update_entry(
            entry, data=data, options=options, version=2
        )
        _LOGGER.debug("Migrated WordClock config entry to version 2")

    return True


def _async_remove_legacy_entities(
    hass: HomeAssistant, entry: WordClockConfigEntry
) -> None:
    """Drop the extra word switches and device row created by version 1.x.

    Extra words are lights now, so the old switches would linger forever as
    unavailable entities next to an empty duplicate device.
    """
    registry = er.async_get(hass)
    for registry_entry in er.async_entries_for_config_entry(registry, entry.entry_id):
        if registry_entry.domain == "switch" and LEGACY_SWITCH_UNIQUE_ID.search(
            registry_entry.unique_id
        ):
            registry.async_remove(registry_entry.entity_id)

    host = entry.data.get(CONF_HOST) or entry.data.get(LEGACY_CONF_IP_ADDRESS)
    if not host:
        return
    device_registry = dr.async_get(hass)
    legacy_id = f"wordclock_{host.replace('.', '_')}"
    if device := device_registry.async_get_device(identifiers={(DOMAIN, legacy_id)}):
        device_registry.async_remove_device(device.id)


def _async_rekey_registry(hass: HomeAssistant, entry: WordClockConfigEntry) -> None:
    """Move entities and the device onto the MAC-based identifier.

    Entities are namespaced by the config entry unique id, which is the device
    MAC. Installations created before that id existed used the config entry id
    instead; remap them so nothing is orphaned.
    """
    if not entry.unique_id or entry.unique_id == entry.entry_id:
        return

    old_prefix = f"{entry.entry_id}_"
    new_prefix = f"{entry.unique_id}_"

    registry = er.async_get(hass)
    for registry_entry in er.async_entries_for_config_entry(registry, entry.entry_id):
        if not registry_entry.unique_id.startswith(old_prefix):
            continue
        new_unique_id = new_prefix + registry_entry.unique_id[len(old_prefix) :]
        if registry.async_get_entity_id(
            registry_entry.domain, DOMAIN, new_unique_id
        ):
            # A rekeyed entity already exists; drop the leftover.
            registry.async_remove(registry_entry.entity_id)
            continue
        registry.async_update_entity(
            registry_entry.entity_id, new_unique_id=new_unique_id
        )

    device_registry = dr.async_get(hass)
    old_identifiers = {(DOMAIN, entry.entry_id)}
    if device := device_registry.async_get_device(identifiers=old_identifiers):
        device_registry.async_update_device(
            device.id, new_identifiers={(DOMAIN, entry.unique_id)}
        )
