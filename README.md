# Switchify

Switch your keyboard layout with the **Caps Lock** key.

A tiny, portable Windows utility written in plain C (Win32 API, no external dependencies). A single executable — settings and logs live next to it, no installer required.

> **Fork of [erryox/Switchy](https://github.com/erryox/Switchy)** (MIT), which was archived in September 2024. Switchify continues and extends it with a tray icon, an on-screen layout indicator, an auto-return mode, a localized interface, and more.

## Features

- **Caps Lock** switches the keyboard layout; **Shift+Caps Lock** still toggles the real Caps Lock state.
- **Tray icon** showing the current layout as a two-letter language code. Double-click it to switch the layout.
- **On-screen layout indicator** — a thin colored overlay bar that appears whenever the active layout isn't your primary one. Configurable from the tray menu: color (fixed presets, a custom color, or a **Rainbow** mode that auto-cycles high-contrast colors to catch your eye), thickness, length, alignment, and which **screen edge(s)** it shows on — pick several to display the bar on multiple edges at once.
- **Primary layout mode** — automatically returns to your primary layout after a configurable idle timeout.
- **Countdown indicator** — the overlay bar shrinks as the auto-return timer counts down.
- **Multilingual interface** (English / Russian), switchable from the tray menu.
- **Run at startup**, plus enable/disable, all from the tray menu.
- **Portable** — file logging and an `.ini` config are kept next to the executable. No registry clutter beyond the optional startup entry, no dependencies.

## Installation & usage

1. Grab `Switchify.exe` from the [latest release](https://github.com/JD72/Switchify/releases/latest), or build it yourself (see below).
2. Put it wherever you like and run it. To start it with Windows, enable **Run at startup** from the tray icon menu.
3. Right-click the tray icon for all options.

**Shortcuts**

| Shortcut | Action |
|---|---|
| `Caps Lock` | Switch keyboard layout |
| `Shift + Caps Lock` | Toggle Caps Lock state |
| Double-click tray icon | Switch keyboard layout |

> **Note:** to switch layouts in programs running as administrator, Switchify must also run as administrator. This can be automated with Task Scheduler.

## Build

- **Requirements:** Windows 10/11 (x64), Visual Studio 2022 with the MSVC toolset **v143**. No external dependencies.
- **Build (Release, x64):**

  ```
  msbuild Switchify.sln /p:Configuration=Release /p:Platform=x64
  ```

- **Output:** `x64/Release/Switchify.exe`. The `en.lng` / `ru.lng` localization files are copied next to the executable automatically by a post-build step.

## License

MIT — see [LICENSE](LICENSE). Original work © 2024 Max Ignatiev; changes in this fork © 2026 JD72.
