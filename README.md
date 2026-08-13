# AWSW WordClock Integration for Home Assistant

Home Assistant integration for the [AWSW WordClock 16x16](https://www.printables.com/model/768062-wordclock-16x16-2024).
It talks to the clock's local JSON API, so everything works without a cloud account.

![WordClock device page](https://github.com/user-attachments/assets/9bc1ace4-eee2-4e9e-99b6-e08400a31fa1)
![WordClock entities](https://github.com/user-attachments/assets/0dac0164-196d-40bb-881a-427a641b7176)

## Requirements

- An AWSW WordClock running **firmware V5.0 or newer** (the JSON API under `/api/` must be present).
- Home Assistant 2025.2 or newer.

## Installation

### HACS (recommended)

1. Add this repository to HACS as a custom repository.
2. Install "AWSW WordClock".
3. Restart Home Assistant.

### Manual

1. Copy the `custom_components/awsw_wordclock` folder into your Home Assistant `custom_components` directory.
2. Restart Home Assistant.

## Setup

`Settings > Devices & Services > Add Integration > AWSW WordClock`, then enter the IP address
or hostname of the clock. Nothing else is needed — the words, colours and settings are read
from the device. If the clock later changes its address, use **Reconfigure** on the entry;
the device is recognised by its MAC address.

## Entities

Everything is polled from `/api/status` every 30 seconds, so the states stay correct even
when the clock is operated through its own web interface.

### Lights

| Entity | What it does |
| --- | --- |
| One light per extra word | On/off plus colour. Brightness is applied by scaling the colour, because the firmware stores only a raw RGB value per word. |
| Time / Time (night) | Colour and brightness of the time display. |
| Background / Background (night) | Colour and brightness of the background. |

Brightness is scaled to the firmware's `intensityLimit`, so Home Assistant never sends a
value the clock would reject.

### Scrolling text

A `notify` entity sends a single line of text to the clock:

```yaml
action: notify.send_message
target:
  entity_id: notify.wordclock_scrolling_text
data:
  message: "Dinner is ready"
```

The text is percent-encoded, so umlauts and special characters work and no quoting is needed.
An optional `title` is prepended to the message, since the clock renders one line.

### Switches (configuration)

Night mode, single minutes, smooth transition, digital time on the hour, random daily colours,
startup text, show IP on startup.

### Buttons

Restart, LED test, digital time test, reset word display, turn all extra words off,
check for updates, optimise Wi-Fi, and sync the clock to Home Assistant's time.

### Diagnostics

Firmware version, available version, IP, Wi-Fi network and signal, device time, day/night
status, NTP status and server, clock language, current brightness, plus binary sensors for
time sync, online mode, update available and night mode.

Several of these are disabled by default; enable them on the device page if you want them.

## Upgrading from 1.x

Version 2.0 is a rewrite against the firmware's V5 API and **replaces the old switches**:

- Extra words are now `light` entities with colour support instead of `switch` entities.
  Automations referring to the old `switch.word_*` entities need to be pointed at the new
  `light.*` entities.
- The language dropdown is gone. The clock reports its own words, so the list is always
  correct — including languages the old integration did not know about.
- The config entry is migrated automatically; the address moves from `ip_address` to `host`.

## Device API reference

Two HTTP interfaces are available on the clock. This integration uses the first one.

### Port 80 — `/api/` (JSON)

| Endpoint | Purpose |
| --- | --- |
| `GET /api/status` | Full device state, including `extraWords[]` with `id`, `name`, `active` and `color`. |
| `GET /api/preview` | Live matrix preview: `time`, `lang`, palette and a 256-character pixel string. |
| `GET /api/set?<key>=<value>` | Writes one or more settings. Booleans use `1`/`0`, colours use `#RRGGBB`. |
| `GET /api/action?cmd=<cmd>` | `restart`, `test`, `digitalTimeTest`, `wordReset`, `resetExtraWords`, `updateCheck`, `updateNow`, `wifiOptimize`. |
| `GET /api/ticker?text=<text>&color=<#RRGGBB>` | Shows scrolling text. |
| `GET /api/time?value=<ISO8601>` | Sets the clock time. |

Writable `set` keys include `ew<N>` and `ewColor<N>` for extra words, `timeColor`,
`backColor`, `timeColorNight`, `backColorNight`, the brightness values
`timeBrightnessDay`, `timeBrightnessNight`, `backBrightnessDay`, `backBrightnessNight`,
`tickerBrightness`, `tickerColor`, the booleans `nightMode`, `singleMinutes`,
`smoothTransition`, `digitalHourChime`, `randomDayColors`, `startupAnimation`, `showIp`,
`onlineMode`, the times `dayStart` and `dayStop`, and `timeServer`, `timeZone`,
`hourOffset`, `autoUpdateMode`, `autoUpdateHour`, `autoUpdateMinute`, `wifiMaxTries`,
`ntpMaxTries`, `language`, `board`.

### Port 2023 — Smart Home URLs (plain text)

Kept by the firmware for existing smart home setups; not used by this integration.

| Endpoint | Purpose |
| --- | --- |
| `GET /status` | `R-Time=… G-Time=… INTENSITY=… ew1=0 … ew12=0` |
| `GET /ewstatus/?<N>` | `0` or `1` for one extra word. |
| `GET /ewrgb/?<N>` | `ew1: R=143 G=0 B=0` |
| `GET /ew/?ew<N>=1&R=&G=&B=` | Switches words and sets their colour; several words per call. |
| `GET /config?R-Time=&G-Time=&B-Time=&R-Back=&G-Back=&B-Back=&INTENSITY=&INTENSITYviaWEB=` | Colour and brightness. |
| `GET /intensity` | Current brightness. |
| `GET /?text=<text>` | Scrolling text. |
| `GET /resetew1`, `/resetew0`, `/resetewstatus` | Bulk extra word control. |

## Troubleshooting

- The integration needs firmware V5 or newer. On older firmware `/api/status` does not exist
  and setup fails with "does not look like a WordClock".
- If entities go unavailable, the clock could not be reached for a poll; check that its IP
  has not changed and use **Reconfigure** if it has.

## Credits

Created for the [AWSW WordClock](https://www.printables.com/model/768062-wordclock-16x16-2024/)
community.

## License

MIT — see [LICENSE](LICENSE).
