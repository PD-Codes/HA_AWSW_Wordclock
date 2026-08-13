"""Data update coordinator for the AWSW WordClock."""

from __future__ import annotations

import logging
from datetime import timedelta
from typing import Any

from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed

from .api import WordClockApi, WordClockError
from .const import (
    DEFAULT_INTENSITY_LIMIT,
    DEFAULT_SCAN_INTERVAL_SECONDS,
    DOMAIN,
    EXTRA_WORD_COUNT,
    KEY_EXTRA_WORDS,
    KEY_INTENSITY_LIMIT,
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
            return await self.api.async_get_status()
        except WordClockError as err:
            raise UpdateFailed(str(err)) from err

    async def async_set(self, **values: Any) -> None:
        """Write settings and refresh so every entity sees the new state."""
        try:
            await self.api.async_set(**values)
        except WordClockError as err:
            raise UpdateFailed(str(err)) from err
        await self.async_request_refresh()

    async def async_action(self, command: str) -> None:
        """Run a maintenance command and refresh afterwards."""
        try:
            await self.api.async_action(command)
        except WordClockError as err:
            raise UpdateFailed(str(err)) from err
        await self.async_request_refresh()

    @property
    def intensity_limit(self) -> int:
        """Return the maximum brightness the firmware accepts."""
        limit = self.data.get(KEY_INTENSITY_LIMIT) if self.data else None
        if isinstance(limit, int) and limit > 0:
            return limit
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

    @property
    def known_extra_word_ids(self) -> set[int]:
        """Return every extra word id the firmware could ever report."""
        return set(range(1, EXTRA_WORD_COUNT + 1))
