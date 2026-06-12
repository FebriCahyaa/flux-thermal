#!/system/bin/sh
#
# Copyright (C) 2024-2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0

MODDIR=$(dirname "$0")
MODULE_CONFIG="/data/adb/.config/flux_thermal"
FLUX_MODPATH="/data/adb/modules/flux"
FLUX_CONFIG="/data/adb/.config/flux"
CLEANUP_SCRIPT="/data/adb/service.d/.flux_thermal_cleanup.sh"

# ── Restore original module.prop ──────────────────────────────────────────────

[ -f "$MODDIR/module.prop.orig" ] && \
    cp "$MODDIR/module.prop.orig" "$MODDIR/module.prop"

# ── Rotate logs ───────────────────────────────────────────────────────────────

[ -f "$MODULE_CONFIG/flux_thermal.log" ] && \
    mv "$MODULE_CONFIG/flux_thermal.log" "$MODULE_CONFIG/flux_thermal.log.prev"

# ── Create cleanup script ─────────────────────────────────────────────────────

[ ! -f "$CLEANUP_SCRIPT" ] && {
    mkdir -p "$(dirname "$CLEANUP_SCRIPT")"
    cp "$MODDIR/cleanup.sh" "$CLEANUP_SCRIPT"
    chmod +x "$CLEANUP_SCRIPT"
}

# ── Runtime dependency check ──────────────────────────────────────────────────
# Flux may be removed after Flux Thermal was installed.
# If Flux is gone, disable ourselves until next boot.

check_flux_dependency() {
    # Flux module dir must exist
    [ ! -d "$FLUX_MODPATH" ] && return 1
    # Flux must not be disabled
    [ -d "$FLUX_MODPATH/disable" ] && return 1
    # Flux config dir must exist (daemon has run at least once since Flux install)
    [ ! -d "$FLUX_CONFIG" ] && return 1
    return 0
}

if ! check_flux_dependency; then
    # Write a sentinel for the WebUI / daemon to read
    mkdir -p "$MODULE_CONFIG"
    echo "flux_missing" > "$MODULE_CONFIG/dependency_error"

    # Patch module.prop to show the error
    sed -i 's/^description=.*/description=[❌ Flux Tweaks not found] Install Flux Tweaks first./' \
        "$MODDIR/module.prop" 2>/dev/null

    # Exit — don't start the daemon
    exit 0
fi

# Clear any previous dependency error
rm -f "$MODULE_CONFIG/dependency_error"

# ── Wait for boot ─────────────────────────────────────────────────────────────

while [ -z "$(getprop sys.boot_completed)" ]; do
    sleep 30
done

# ── Create config dir if missing ──────────────────────────────────────────────

mkdir -p "$MODULE_CONFIG"

# ── Write default config if absent ───────────────────────────────────────────

if [ ! -f "$MODULE_CONFIG/config.json" ]; then
    cat > "$MODULE_CONFIG/config.json" << 'JSONEOF'
{
  "preferences": {
    "enabled": true,
    "aggressive_mode": false,
    "protect_battery": true,
    "cool_down_on_charge": true,
    "log_level": 4,
    "threshold_cool_mc": 0,
    "threshold_warm_mc": 0,
    "threshold_hot_mc": 0,
    "threshold_critical_mc": 0
  },
  "zone_filter": {
    "include_types": ["cpu", "gpu", "soc", "skin", "thermal"]
  }
}
JSONEOF
fi

# ── Start the daemon ───────────────────────────────────────────────────────────

DAEMON="$MODDIR/system/bin/flux_thermald"

[ ! -f "$DAEMON" ] && {
    echo "flux_thermald binary not found: $DAEMON" > "$MODULE_CONFIG/daemon_error"
    exit 1
}

chmod +x "$DAEMON"
"$DAEMON" daemon
