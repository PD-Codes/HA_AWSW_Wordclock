"""The AWSW WordClock integration."""

from __future__ import annotations

import logging

from homeassistant.core import HomeAssistant
from homeassistant.exceptions import ConfigEntryNotReady
from homeassistant.helpers.aiohttp_client import async_get_clientsession

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


async def async_setup_entry(hass: HomeAssistant, entry: WordClockConfigEntry) -> bool:
    """Set up AWSW WordClock from a config entry."""
    api = WordClockApi(entry.data[CONF_HOST], async_get_clientsession(hass))
    coordinator = WordClockCoordinator(hass, entry, api)
    await coordinator.async_config_entry_first_refresh()

    # Adopt the MAC as the entry unique id the first time we see it, so that a
    # device that changed its IP can still be recognised.
    if entry.unique_id is None and (mac := coordinator.data.get(KEY_MAC)):
        hass.config_entries.async_update_entry(entry, unique_id=_format_mac(mac))

    entry.runtime_data = coordinator
    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    entry.async_on_unload(entry.add_update_listener(_async_update_listener))
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

        hass.config_entries.async_update_entry(
            entry, data=data, options=options, version=2
        )
        _LOGGER.debug("Migrated WordClock config entry to version 2")

    return True


async def _async_update_listener(
    hass: HomeAssistant, entry: WordClockConfigEntry
) -> None:
    """Reload the entry when its options change."""
    await hass.config_entries.async_reload(entry.entry_id)


def _format_mac(mac: str) -> str:
    """Return a MAC address in the lowercase colon-separated form HA uses."""
    return mac.replace("-", ":").lower()
