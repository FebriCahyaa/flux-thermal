# Flux Thermal

Adaptive thermal management module for Android — designed to work alongside **Flux Tweaks**.

## Requirements

- Flux Tweaks (`id=flux`) **must be installed and enabled** before installing Flux Thermal.
- Android 9 (API 28) or higher.
- Magisk / KernelSU / APatch.
- arm64 or arm device.

## Features

- Monitors all `/sys/class/thermal/thermal_zone*` sysfs nodes
- 5 adaptive profiles: Normal → Cool → Warm → Hot → Critical
- Hysteresis to prevent rapid oscillation between profiles
- Per-SoC optimisations: MediaTek, Snapdragon, Exynos, Unisoc, Google Tensor, Tegra, Kirin
- GKI / Non-GKI kernel detection (inherits Flux's cached result)
- Charging-aware: optional COOL floor while plugged in
- WebUI with live sensor dashboard, threshold configuration, and zone filter
- Logs and bug report export

## Integration with Flux

Flux Thermal reads:
- `/data/adb/.config/flux/soc_recognition` — SoC type (set by Flux at install)
- `/data/adb/.config/flux/is_gki` — Kernel type (set by Flux at boot)
- `/data/adb/.config/flux/synthesis_core.json` — Charging state, screen state

Flux Thermal **will not install or start** if Flux Tweaks is absent or disabled.

## License

Apache License 2.0 — Copyright (C) 2024-2026 FebriCahyaa
