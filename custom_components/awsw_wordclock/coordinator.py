"""Data update coordinator for the AWSW WordClock."""

from __future__ import annotations

import logging
from datetime import timedelta
from typing import Any

from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import HomeAssistantError
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed

from .api import WordClockApi, WordClockError
from .const import (
    DEFAULT_INTENSITY_LIMIT,
    DEFAULT_SCAN_INTERVAL_SECONDS,
    DOMAIN,
    FAST_SCAN_INTERVAL_SECONDS,
    KEY_EXTRA_WORDS,
    KEY_INTENSITY_LIMIT,
    is_custom_firmware,
)

_LOGGER = logging.getLogger(__name__)

# Config entry carrying the coordinator in its runtime data.
WordClockConfigEntry = ConfigEntry["WordClockCoordinator"]


class WordClockCoordinator(DataUpdateCoordinator[dict[str, Any]]):
    """Poll ``/api/status`` and share the result with every platform."""

    config_entry: WordClockConfigEntry

    def __init__(
        self,
        hass: HomeAssistant,
        entry: WordClockConfigEntry,
        api: WordClockApi,
    ) -> None:
        """Initialise the coordinator."""
        super().__init__(
            hass,
            _LOGGER,
            name=DOMAIN,
            config_entry=entry,
            update_interval=timedelta(seconds=DEFAULT_SCAN_INTERVAL_SECONDS),
        )
        self.api = api

    async def _async_update_data(self) -> dict[str, Any]:
        """Fetch the current device state."""
        try:
            status = await self.api.async_get_status()
        except WordClockError as err:
            raise UpdateFailed(str(err)) from err

        # Pick the poll rate by firmware; re-evaluated on every poll so a firmware
        # change is picked up without reloading the integration.
        seconds = (
            FAST_SCAN_INTERVAL_SECONDS
            if is_custom_firmware(status)
            else DEFAULT_SCAN_INTERVAL_SECONDS
        )
        self.update_interval = timedelta(seconds=seconds)
        return status

    async def async_set(self, **values: Any) -> None:
        """Write settings and make the new state visible straight away."""
        try:
            await self.api.async_set(**values)
        except WordClockError as err:
            raise HomeAssistantError(
                f"WordClock did not accept the change: {err}"
            ) from err

        # Anything the optimistic map covers is applied without hitting the
        # device again. Polling right here would bypass the refresh debouncer
        # and could read back a value the clock has not applied yet.
        optimistic = _as_status(values)
        mapped = len(optimistic)
        if self.data and len(optimistic) < len(values):
            words, complete = _apply_extra_words(self.extra_words, values)
            optimistic[KEY_EXTRA_WORDS] = words
            if complete:
                mapped = len(values)
        if self.data and mapped == len(values):
            self.async_set_updated_data({**self.data, **optimistic})
            return

        if self.data and optimistic:
            self.data.update(optimistic)
        await self.async_request_refresh()

    async def async_action(self, command: str, refresh: bool = True) -> None:
        """Run a maintenance command, optionally refreshing afterwards."""
        try:
            await self.api.async_action(command)
        except WordClockError as err:
            raise HomeAssistantError(f"WordClock rejected {command}: {err}") from err
        if refresh:
            await self.async_request_refresh()

    @property
    def intensity_limit(self) -> int:
        """Return the maximum brightness the firmware accepts."""
        limit = self.data.get(KEY_INTENSITY_LIMIT) if self.data else None
        if isinstance(limit, (int, float)) and limit > 0:
            return int(limit)
        return DEFAULT_INTENSITY_LIMIT

    @property
    def extra_words(self) -> list[dict[str, Any]]:
        """Return the extra word slots reported by the device."""
        words = (self.data or {}).get(KEY_EXTRA_WORDS)
        if not isinstance(words, list):
            return []
        return [word for word in words if isinstance(word, dict) and "id" in word]

    def extra_word(self, word_id: int) -> dict[str, Any] | None:
        """Return a single extra word slot by its firmware id."""
        for word in self.extra_words:
            if word.get("id") == word_id:
                return word
        return None


def _as_status(values: dict[str, Any]) -> dict[str, Any]:
    """Translate written values into their /api/status representation.

    Only the flat keys are mapped; extra word writes are handled by
    ``async_set`` because they live in a nested list.
    """
    return {key: value for key, value in values.items() if not key.startswith("ew")}


def _apply_extra_words(
    words: list[dict[str, Any]], values: dict[str, Any]
) -> tuple[list[dict[str, Any]], bool]:
    """Apply ``ew<N>`` / ``ewColor<N>`` writes to a copy of the extra word list.

    Returns the new list and whether every extra word write could be mapped.
    """
    by_id = {word.get("id"): dict(word) for word in words}
    complete = True
    for key, value in values.items():
        if not key.startswith("ew"):
            continue
        if key.startswith("ewColor"):
            word_id, field, new = key[7:], "color", value
        elif key.startswith("ew") and str(value) in ("0", "1", "True", "False"):
            word_id, field, new = key[2:], "active", str(value) in ("1", "True")
        else:
            complete = False
            continue
        try:
            word = by_id.get(int(word_id))
        except ValueError:
            word = None
        if word is None:
            complete = False
            continue
        word[field] = new
    return [by_id.get(word.get("id"), word) for word in words], complete
