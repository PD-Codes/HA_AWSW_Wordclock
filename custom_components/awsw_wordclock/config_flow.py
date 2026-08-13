"""Config flow for the AWSW WordClock integration."""

from __future__ import annotations

import ipaddress
import logging
import re
from typing import Any

import voluptuous as vol
from homeassistant.config_entries import ConfigFlow, ConfigFlowResult
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.device_registry import format_mac
from homeassistant.helpers.selector import (
    TextSelector,
    TextSelectorConfig,
    TextSelectorType,
)

from .api import WordClockApi, WordClockConnectionError, WordClockError
from .const import CONF_HOST, DOMAIN, KEY_MAC

_LOGGER = logging.getLogger(__name__)

STEP_USER_SCHEMA = vol.Schema(
    {
        vol.Required(CONF_HOST): TextSelector(
            TextSelectorConfig(type=TextSelectorType.TEXT, autocomplete="off")
        ),
    }
)

HOSTNAME_LABEL = re.compile(r"^[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?$", re.IGNORECASE)


class WordClockConfigFlow(ConfigFlow, domain=DOMAIN):
    """Handle a config flow for AWSW WordClock."""

    VERSION = 2

    async def async_step_user(
        self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        """Handle the initial step."""
        errors: dict[str, str] = {}

        if user_input is not None:
            host = user_input[CONF_HOST].strip()
            status, errors = await self._async_probe(host)

            if status is not None:
                if mac := status.get(KEY_MAC):
                    await self.async_set_unique_id(format_mac(mac))
                    self._abort_if_unique_id_configured(updates={CONF_HOST: host})
                else:
                    self._async_abort_entries_match({CONF_HOST: host})

                return self.async_create_entry(
                    title=status.get("hostname") or f"WordClock ({host})",
                    data={CONF_HOST: host},
                )

        return self.async_show_form(
            step_id="user",
            data_schema=self.add_suggested_values_to_schema(
                STEP_USER_SCHEMA, user_input or {}
            ),
            errors=errors,
        )

    async def async_step_reconfigure(
        self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        """Let the user point an existing entry at a new address."""
        entry = self._get_reconfigure_entry()
        errors: dict[str, str] = {}

        if user_input is not None:
            host = user_input[CONF_HOST].strip()
            status, errors = await self._async_probe(host)

            if status is not None:
                unique_id = entry.unique_id
                if mac := status.get(KEY_MAC):
                    unique_id = format_mac(mac)
                    await self.async_set_unique_id(unique_id)
                    if entry.unique_id is not None:
                        self._abort_if_unique_id_mismatch(reason="wrong_device")
                    else:
                        # Entries created by version 1.x have no id yet, so make
                        # sure no other entry has already claimed this device.
                        self._abort_if_unique_id_configured()

                return self.async_update_reload_and_abort(
                    entry,
                    unique_id=unique_id,
                    data_updates={CONF_HOST: host},
                )

        return self.async_show_form(
            step_id="reconfigure",
            data_schema=self.add_suggested_values_to_schema(
                STEP_USER_SCHEMA, user_input or {CONF_HOST: entry.data.get(CONF_HOST)}
            ),
            errors=errors,
        )

    async def _async_probe(
        self, host: str
    ) -> tuple[dict[str, Any] | None, dict[str, str]]:
        """Contact the device, returning its status or the error to show."""
        if not _is_valid_host(host):
            return None, {"base": "invalid_host"}

        try:
            api = WordClockApi(host, async_get_clientsession(self.hass))
            status = await api.async_get_status()
        except WordClockConnectionError:
            return None, {"base": "cannot_connect"}
        except WordClockError:
            return None, {"base": "unknown_device"}
        except Exception:  # noqa: BLE001 - surfaced as a generic error
            _LOGGER.exception("Unexpected error probing WordClock at %s", host)
            return None, {"base": "unknown"}

        # A device that does not report these is not running firmware V5.
        if "extraWords" not in status or "version" not in status:
            return None, {"base": "unknown_device"}

        return status, {}


def _is_valid_host(host: str) -> bool:
    """Return whether the given string is a usable IP address or hostname."""
    if not host or len(host) > 253 or "://" in host or "/" in host:
        return False
    try:
        ipaddress.ip_address(host)
    except ValueError:
        return all(HOSTNAME_LABEL.match(label) for label in host.split("."))
    return True
