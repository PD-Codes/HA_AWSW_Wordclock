"""Config flow for the AWSW WordClock integration."""

from __future__ import annotations

import ipaddress
import logging
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

            if not _is_valid_host(host):
                errors["base"] = "invalid_host"
            else:
                api = WordClockApi(host, async_get_clientsession(self.hass))
                try:
                    status = await api.async_get_status()
                except WordClockConnectionError:
                    errors["base"] = "cannot_connect"
                except WordClockError:
                    errors["base"] = "unknown_device"
                except Exception:  # noqa: BLE001 - surfaced as a generic error
                    _LOGGER.exception("Unexpected error probing WordClock at %s", host)
                    errors["base"] = "unknown"
                else:
                    if mac := status.get(KEY_MAC):
                        await self.async_set_unique_id(format_mac(mac))
                        self._abort_if_unique_id_configured(
                            updates={CONF_HOST: host}
                        )
                    else:
                        self._async_abort_entries_match({CONF_HOST: host})

                    title = status.get("hostname") or f"WordClock ({host})"
                    return self.async_create_entry(
                        title=title, data={CONF_HOST: host}
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
            if not _is_valid_host(host):
                errors["base"] = "invalid_host"
            else:
                api = WordClockApi(host, async_get_clientsession(self.hass))
                try:
                    status = await api.async_get_status()
                except WordClockConnectionError:
                    errors["base"] = "cannot_connect"
                except WordClockError:
                    errors["base"] = "unknown_device"
                else:
                    if mac := status.get(KEY_MAC):
                        await self.async_set_unique_id(format_mac(mac))
                        self._abort_if_unique_id_mismatch(reason="wrong_device")
                    return self.async_update_reload_and_abort(
                        entry, data_updates={CONF_HOST: host}
                    )

        return self.async_show_form(
            step_id="reconfigure",
            data_schema=self.add_suggested_values_to_schema(
                STEP_USER_SCHEMA, {CONF_HOST: entry.data.get(CONF_HOST)}
            ),
            errors=errors,
        )


def _is_valid_host(host: str) -> bool:
    """Return whether the given string is a usable IP address or hostname."""
    if not host:
        return False
    try:
        ipaddress.ip_address(host)
    except ValueError:
        # Not an IP: accept it as a hostname if it looks sane.
        return all(part and len(part) <= 63 for part in host.split("."))
    return True
