#!/system/bin/sh
#
# Copyright (C) 2024-2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0

# shellcheck disable=SC2317,SC3006,SC3018,SC3034,SC3057,SC3037

MODULE_CONFIG="/data/adb/.config/flux_thermal"
FLUX_CONFIG="/data/adb/.config/flux"

# ── Profile name helper ────────────────────────────────────────────────────────

profile_name() {
    case "$1" in
    0) echo "NORMAL" ;;
    1) echo "COOL" ;;
    2) echo "WARM" ;;
    3) echo "HOT" ;;
    4) echo "CRITICAL" ;;
    *) echo "UNKNOWN ($1)" ;;
    esac
}

# ── SOC name helper ────────────────────────────────────────────────────────────

soc_name() {
    case "$1" in
    1) echo "MediaTek" ;;
    2) echo "Snapdragon" ;;
    3) echo "Exynos" ;;
    4) echo "Unisoc" ;;
    5) echo "Google Tensor" ;;
    6) echo "Nvidia Tegra" ;;
    7) echo "Kirin" ;;
    0) echo "Unknown" ;;
    *) echo "Unknown ($1)" ;;
    esac
}

# ── Logcat ────────────────────────────────────────────────────────────────────

logcat() {
    echo -ne "\e[H\e[2J\e[3J"
    trap 'echo -ne "\e[H\e[2J\e[3J"; exit 0' INT

    SOC_CODE="0"
    [ -f "$FLUX_CONFIG/soc_recognition" ] && SOC_CODE=$(cat "$FLUX_CONFIG/soc_recognition")

    while true; do
        echo -ne "\e[H"
        echo "╔══════════════════════════════════════════════╗"
        echo "║           Flux Thermal — Live Monitor         ║"
        echo "╠══════════════════════════════════════════════╣"

        PROFILE="?"
        [ -f "$MODULE_CONFIG/current_thermal_profile" ] && \
            PROFILE=$(profile_name "$(cat "$MODULE_CONFIG/current_thermal_profile")")
        echo "║  Profile  : $PROFILE"
        echo "║  SoC      : $(soc_name "$SOC_CODE")"
        echo "║  Kernel   : $(uname -r | cut -c1-40)"
        echo "╠══════════════════════════════════════════════╣"
        echo "║  Thermal Zones:"
        for dir in /sys/class/thermal/thermal_zone*; do
            TYPE=$(cat "$dir/type" 2>/dev/null)
            TEMP=$(cat "$dir/temp" 2>/dev/null)
            [ -z "$TEMP" ] && continue
            TEMP_C=$(awk "BEGIN{printf \"%.1f\", $TEMP/1000}")
            POLICY=$(cat "$dir/policy" 2>/dev/null)
            printf "║    %-28s %5s°C  [%s]\n" "$TYPE" "$TEMP_C" "$POLICY"
        done
        echo "╚══════════════════════════════════════════════╝"
        sleep 2
    done
}

# ── Bug report ────────────────────────────────────────────────────────────────

save_logs() {
    report_dir="$MODULE_CONFIG/flux_thermal_bugreport_temp"
    mkdir -p "$report_dir"

    log_file="flux_thermal_bugreport_$(date +"%Y-%m-%d_%H_%M").tar.gz"
    SOC="Unknown"
    [ -f "$FLUX_CONFIG/soc_recognition" ] && \
        SOC=$(soc_name "$(cat "$FLUX_CONFIG/soc_recognition")")

    {
        echo "*****************************************************"
        echo "Flux Thermal Bug Report"
        echo "Module Version: $(awk -F'=' '/version=/ {print $2}' /data/adb/modules/flux_thermal/module.prop 2>/dev/null)"
        echo "Chipset: $SOC $(getprop ro.board.platform)"
        echo "Fingerprint: $(getprop ro.build.fingerprint)"
        echo "Android SDK: $(getprop ro.build.version.sdk)"
        echo "Kernel: $(uname -r -m)"
        echo "GKI: $(cat "$FLUX_CONFIG/is_gki" 2>/dev/null)"
        echo "*****************************************************"
        echo ""
        [ -f "$MODULE_CONFIG/flux_thermal.log" ] && cat "$MODULE_CONFIG/flux_thermal.log"
    } >"$report_dir/flux_thermal.log"

    cp -r /sys/fs/pstore/. "$report_dir/pstore/" 2>/dev/null || true

    (
        cd "$report_dir" || exit
        tar -czf "$log_file" .
    )

    target_dir="/sdcard/Download"
    [ ! -d "$target_dir" ] && mkdir -p "$target_dir"

    if [ -f "$report_dir/$log_file" ]; then
        cp "$report_dir/$log_file" "$target_dir/$log_file"
        echo "$target_dir/$log_file"
        rm -rf "$report_dir"
        return 0
    else
        rm -rf "$report_dir"
        return 1
    fi
}

# ── Main ──────────────────────────────────────────────────────────────────────

case "$1" in
"logcat")     logcat ;;
"save_logs")  save_logs ;;
*)
    echo "Usage: flux_thermal_utility {logcat|save_logs}" >&2
    exit 1
    ;;
esac
