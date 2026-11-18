# Mouse Battery Tray

Battery percentage of your wireless mouse in the Windows 11 tray: one number
per mouse, colored by charge, blue while charging, grey while the mouse
sleeps. A low-battery notification appears when the level drops.

It talks to the mouse **the same way the vendor's web driver ("online
software") does**, over USB HID, so neither the vendor app nor a browser tab
has to stay open.

It comes in two forms built from **one source file**:

- **Windhawk mod**: [`mods/mouse-battery-tray.wh.cpp`](mods/mouse-battery-tray.wh.cpp).
  It runs as a Windhawk tool mod in its own process, not inside Explorer.
- **Standalone app**: `MouseBatteryTray.exe` from
  [Releases](../../releases). It needs no install and no Windhawk.

## Supported mice

| Family | Examples | Status |
|---|---|---|
| Attack Shark Ultra / web-driver series (`0x83` feature exchange from the official web driver) | **R5 Ultra**, R11 Ultra, X11 Ultra | protocol taken from the official web driver; please confirm on hardware |
| WLmouse, Lamzu, G-Wolves, Glorious (Sinowealth), MCHOSE A5 Pro Max (same `0x83` exchange) | Beast X Max, Maya X, HSK Pro, Model O | confirmed by other projects |
| Attack Shark Beken dongle (VID 1D57) | X3, X3 Max, X6, X11, R1 | confirmed by other projects |
| MCHOSE | M7 Ultra, L7 Pro, A7 V2 Ultra, G7 | confirmed by other projects (A7 V2 Ultra untested) |
| Compx / Nordic (ATK, VXE, VGN, Pulsar, Scyrox, Dareu, Hitscan, Zaopin, G-Wolves Lycan) | VXE R1 Pro Max, ATK X1 Pro, Pulsar X2 CrazyLight Mini | confirmed by other projects |
| ATK Zero (Nordic 54L15) | ATK Zero | confirmed by other projects |
| Razer | Viper V3/V4 Pro, DeathAdder V3/V4 Pro, Basilisk V3 Pro | OpenRazer protocol |
| Logitech HID++ 2.0 | G Pro X Superlight, G502 X, MX series | Solaar protocol |
| Bluetooth | any mouse in Bluetooth mode (R5 Ultra included) | Windows' own level |

Unknown 2.4 GHz receivers that carry a mouse get the two most common
read-only battery queries, so many unlisted brands work right away. Protocol
details and credits are in [docs/PROTOCOLS.md](docs/PROTOCOLS.md).

**Your mouse isn't detected?** [docs/ADDING_A_MOUSE.md](docs/ADDING_A_MOUSE.md)
shows how to copy the battery protocol from your mouse's web driver in a few
minutes, with a DevTools snippet. You can then use it as a *custom device*
without changing any code.

## Install

### Windhawk

1. Install [Windhawk](https://windhawk.net/).
2. Windhawk > *Create a new mod*, replace the code with
   [`mods/mouse-battery-tray.wh.cpp`](mods/mouse-battery-tray.wh.cpp), then
   *Compile* and *Exit editing mode*.
3. Change settings on the mod's *Settings* tab.

### Standalone

Download `MouseBatteryTray.exe`, put it in a folder you keep (for example
`%LOCALAPPDATA%\MouseBatteryTray`) and run it. Right-click the icon for
*Start with Windows*, *Settings...* (an INI file next to the exe) and *Exit*.

> Windows 11 hides new tray icons in the overflow menu. To keep them visible,
> open Settings > Personalization > Taskbar > Other system tray icons and
> switch *Mouse Battery Tray* (or *Windhawk* for the mod) on.

## Settings

| Setting | Default | Description |
|---|---|---|
| pollInterval | 60 | seconds between reads (10-3600) |
| iconMode | perDevice | `perDevice` or `lowest` (one icon) |
| iconStyle | number | `number` or `battery` gauge |
| colorByLevel | on | green / amber / red, blue when charging |
| lowBattery | 20 | notification threshold in %, 0 = off |
| showWhenNone | on | show a "–" icon when nothing is found |
| bluetooth / logitech / razer | on | per-family switches |
| probeUnknown | on | read-only probe of unknown mouse receivers |
| allDevices | off | also keyboards, headsets... |
| customDevices | – | see [docs/ADDING_A_MOUSE.md](docs/ADDING_A_MOUSE.md) |

## Notes

- If the vendor's web driver is open and connected at the same time, both
  apps share the same HID channel and may briefly read each other's replies.
  Close it when you're done configuring.
- A receiver can't tell "mouse asleep" from "mouse off". The icon then shows
  the last level in grey for up to 15 minutes.
- Windows doesn't let any app open the mouse pointer collection itself. Every
  query uses the vendor collections, the same ones the web drivers use.

## Build

The standalone exe and the Windhawk compile check run in GitHub Actions
([`.github/workflows/build.yml`](.github/workflows/build.yml)). Pushing a
`v*` tag publishes a release. Locally:

```bash
# MinGW-w64 (Windows MSYS2 or Linux cross)
x86_64-w64-mingw32-g++ -std=c++20 -O2 -DUNICODE -D_UNICODE -municode -mwindows -static \
  standalone/main.cpp -o MouseBatteryTray.exe \
  -lsetupapi -lhid -lgdi32 -lshell32 -ladvapi32 -luser32

# protocol tests, any OS
g++ -std=c++17 tests/parsers_test.cpp -o parsers_test && ./parsers_test
```

CMake also works (`cmake -S . -B build && cmake --build build`).

## Submitting the mod to Windhawk

Open a pull request that adds `mods/mouse-battery-tray.wh.cpp`
to [ramensoftware/windhawk-mods](https://github.com/ramensoftware/windhawk-mods)
under `mods/`.

## License

MIT. Protocol knowledge comes from the open-source projects credited in
[docs/PROTOCOLS.md](docs/PROTOCOLS.md).
