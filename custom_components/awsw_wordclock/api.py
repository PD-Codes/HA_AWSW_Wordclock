"""HTTP client for the AWSW WordClock firmware (V5 JSON API)."""

from __future__ import annotations

import asyncio
import logging
from typing import Any

import aiohttp
from yarl import URL

from .const import REQUEST_TIMEOUT_SECONDS

_LOGGER = logging.getLogger(__name__)


class WordClockError(Exception):
    """Base error for the WordClock API."""


class WordClockConnectionError(WordClockError):
    """The device could not be reached."""


class WordClockResponseError(WordClockError):
    """The device answered, but not the way we expect."""


class WordClockApi:
    """Thin async wrapper around the WordClock ``/api`` endpoints.

    The firmware serves a small JSON API on port 80. Every write is a plain GET
    with query parameters; the device answers with
    ``{"ok": bool, "restart": bool, "message": str}``.
    """

    def __init__(self, host: str, session: aiohttp.ClientSession) -> None:
        """Initialise the client for a device reachable at ``host``."""
        self._host = host
        self._session = session
        self._base = URL(f"http://{host}/api/")

    @property
    def host(self) -> str:
        """Return the host this client talks to."""
        return self._host

    async def _request(self, path: str, params: dict[str, Any] | None = None) -> Any:
        """Perform a GET request and return the decoded JSON body."""
        url = self._base / path
        try:
            async with self._session.get(
                url,
                params={k: str(v) for k, v in (params or {}).items()},
                timeout=aiohttp.ClientTimeout(total=REQUEST_TIMEOUT_SECONDS),
            ) as response:
                response.raise_for_status()
                # The firmware sends JSON as text/plain, so do not enforce the
                # content type here.
                return await response.json(content_type=None)
        except asyncio.TimeoutError as err:
            raise WordClockConnectionError(
                f"Timeout talking to WordClock at {self._host}"
            ) from err
        except aiohttp.ClientError as err:
            raise WordClockConnectionError(
                f"Cannot reach WordClock at {self._host}: {err}"
            ) from err
        except ValueError as err:
            raise WordClockResponseError(
                f"WordClock at {self._host} returned invalid JSON"
            ) from err

    async def async_get_status(self) -> dict[str, Any]:
        """Return the full device state from ``/api/status``."""
        data = await self._request("status")
        if not isinstance(data, dict):
            raise WordClockResponseError("Unexpected payload from /api/status")
        return data

    async def async_get_preview(self) -> dict[str, Any]:
        """Return the live matrix preview from ``/api/preview``."""
        data = await self._request("preview")
        if not isinstance(data, dict):
            raise WordClockResponseError("Unexpected payload from /api/preview")
        return data

    async def async_set(self, **values: Any) -> dict[str, Any]:
        """Write one or more settings via ``/api/set``.

        Booleans are converted to the ``1``/``0`` form the firmware expects.
        """
        params = {
            key: int(value) if isinstance(value, bool) else value
            for key, value in values.items()
        }
        result = await self._request("set", params)
        return self._check_result(result, f"set {sorted(params)}")

    async def async_action(self, command: str) -> dict[str, Any]:
        """Trigger a maintenance command via ``/api/action``."""
        result = await self._request("action", {"cmd": command})
        return self._check_result(result, f"action {command}")

    async def async_send_ticker(self, text: str, color: str | None = None) -> dict[str, Any]:
        """Show ``text`` as scrolling text on the clock."""
        params: dict[str, Any] = {"text": text}
        if color:
            params["color"] = color
        result = await self._request("ticker", params)
        return self._check_result(result, "ticker")

    async def async_set_time(self, iso_timestamp: str) -> dict[str, Any]:
        """Push an ISO 8601 timestamp to the clock via ``/api/time``."""
        result = await self._request("time", {"value": iso_timestamp})
        return self._check_result(result, "time")

    @staticmethod
    def _check_result(result: Any, context: str) -> dict[str, Any]:
        """Raise if the firmware reported a failure, otherwise pass it through."""
        if not isinstance(result, dict):
            raise WordClockResponseError(f"Unexpected payload for {context}")
        if result.get("ok") is False:
            message = result.get("message", "unknown error")
            raise WordClockResponseError(f"WordClock rejected {context}: {message}")
        return result
