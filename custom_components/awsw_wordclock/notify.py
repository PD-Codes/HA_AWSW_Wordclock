"""Notify entity sending scrolling text to the AWSW WordClock."""

from __future__ import annotations

from homeassistant.components.notify import NotifyEntity, NotifyEntityFeature
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import HomeAssistantError
from homeassistant.helpers.entity_platform import AddConfigEntryEntitiesCallback

from .api import WordClockError
from .const import KEY_TICKER_COLOR
from .coordinator import WordClockConfigEntry, WordClockCoordinator
from .entity import WordClockEntity

TICKER_KEY = "ticker"


async def async_setup_entry(
    hass: HomeAssistant,
    entry: WordClockConfigEntry,
    async_add_entities: AddConfigEntryEntitiesCallback,
) -> None:
    """Set up the WordClock ticker."""
    async_add_entities([WordClockTicker(entry.runtime_data)])


class WordClockTicker(WordClockEntity, NotifyEntity):
    """Shows a message as scrolling text on the clock."""

    _attr_translation_key = TICKER_KEY
    _attr_supported_features = NotifyEntityFeature.TITLE

    def __init__(self, coordinator: WordClockCoordinator) -> None:
        """Initialise the ticker entity."""
        super().__init__(coordinator, TICKER_KEY)

    async def async_send_message(self, message: str, title: str | None = None) -> None:
        """Send the message to the clock.

        The title is prepended, since the clock only renders a single line.
        """
        text = f"{title}: {message}" if title else message
        color = self._status.get(KEY_TICKER_COLOR)
        try:
            await self.coordinator.api.async_send_ticker(text, color)
        except WordClockError as err:
            raise HomeAssistantError(
                f"Could not send the message to the WordClock: {err}"
            ) from err
