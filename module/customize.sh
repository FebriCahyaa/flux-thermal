#
# Copyright (C) 2024-2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# shellcheck disable=SC1091,SC2034,SC2317
SKIPUNZIP=1

MODULE_CONFIG="/data/adb/.config/flux_thermal"
FLUX_MODPATH="/data/adb/modules/flux"
FLUX_CONFIG="/data/adb/.config/flux"

make_dir() {
    [ ! -d "$1" ] && mkdir -p "$1"
}

make_node() {
    [ ! -f "$2" ] && echo "$1" >"$2"
}

# ── Abort helpers ─────────────────────────────────────────────────────────────

abort_unsupported_arch() {
    ui_print "*********************************************************"
    ui_print "! Unsupported Architecture: $ARCH"
    ui_print "! Flux Thermal requires arm64 or arm."
    abort "*********************************************************"
}

abort_flux_missing() {
    ui_print "*********************************************************"
    ui_print "! Flux Tweaks is not installed!"
    ui_print "! Flux Thermal requires Flux Tweaks to be installed first."
    ui_print "! Please install Flux Tweaks before this module."
    abort "*********************************************************"
}

abort_flux_disabled() {
    ui_print "*********************************************************"
    ui_print "! Flux Tweaks module is disabled!"
    ui_print "! Please enable Flux Tweaks and reboot before installing."
    abort "*********************************************************"
}

abort_corrupted() {
    ui_print "*********************************************************"
    ui_print "! Unable to extract verify.sh!"
    ui_print "! Module may be corrupted. Please re-download."
    abort "*********************************************************"
}

abort_android_version() {
    ui_print "*********************************************************"
    ui_print "! Android version not supported."
    ui_print "! Please use Android 9 (Pie / API 28) or higher."
    abort "*********************************************************"
}

# ── Dependency check: Flux Tweaks ─────────────────────────────────────────────

ui_print "- Checking Flux Tweaks dependency..."

# Check Flux module directory
[ ! -d "$FLUX_MODPATH" ] && abort_flux_missing

# Check Flux module is not disabled
[ -d "$FLUX_MODPATH/disable" ] && abort_flux_disabled

# Check Flux module.prop for id=flux
if ! grep -q "^id=flux$" "$FLUX_MODPATH/module.prop" 2>/dev/null; then
    abort_flux_missing
fi

ui_print "  -> Flux Tweaks: OK"

# ── Android version check ──────────────────────────────────────────────────────

[ "$API" -lt 28 ] && abort_android_version

# ── Integrity check ────────────────────────────────────────────────────────────

ui_print "- Extracting verify.sh"
unzip -o "$ZIPFILE" 'verify.sh' -d "$TMPDIR" >&2
[ ! -f "$TMPDIR/verify.sh" ] && abort_corrupted
source "$TMPDIR/verify.sh"

# ── Extract module files ───────────────────────────────────────────────────────

ui_print "- Extracting module files"
extract "$ZIPFILE" 'module.prop' "$MODPATH"
extract "$ZIPFILE" 'banner.webp' "$MODPATH"
extract "$ZIPFILE" 'service.sh' "$MODPATH"
extract "$ZIPFILE" 'uninstall.sh' "$MODPATH"
extract "$ZIPFILE" 'cleanup.sh' "$MODPATH"
cp "$MODPATH/module.prop" "$MODPATH/module.prop.orig"

# ── Architecture detection ────────────────────────────────────────────────────

case $ARCH in
"arm64") ARCH_TMP="arm64-v8a" ;;
"arm")   ARCH_TMP="armeabi-v7a" ;;
*)       abort_unsupported_arch ;;
esac

# ── Extract daemon binary ──────────────────────────────────────────────────────

ui_print "- Extracting flux_thermald ($ARCH)"
extract "$ZIPFILE" "libs/$ARCH_TMP/flux_thermald" "$TMPDIR"
cp "$TMPDIR/libs/$ARCH_TMP/flux_thermald" "$MODPATH/system/bin/flux_thermald"
rm -rf "$TMPDIR/libs"

# ── Extract scripts ────────────────────────────────────────────────────────────

ui_print "- Extracting scripts"
extract "$ZIPFILE" 'system/bin/flux_thermal_profiler' "$MODPATH"
extract "$ZIPFILE" 'system/bin/flux_thermal_utility' "$MODPATH"

# ── Skip mountify ─────────────────────────────────────────────────────────────

touch "$MODPATH/skip_mountify"

if [ "$KSU" = "true" ] || [ "$APATCH" = "true" ]; then
    ui_print "- KSU/AP detected, using skip_mount"
    touch "$MODPATH/skip_mount"

    manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"
    BIN_PATH="/data/adb/modules/flux_thermal/system/bin"
    for dir in $manager_paths; do
        [ -d "$dir" ] && {
            ui_print "- Creating symlinks in $dir"
            ln -sf "$BIN_PATH/flux_thermald"            "$dir/flux_thermald"
            ln -sf "$BIN_PATH/flux_thermal_profiler"    "$dir/flux_thermal_profiler"
            ln -sf "$BIN_PATH/flux_thermal_utility"     "$dir/flux_thermal_utility"
        }
    done
fi

# ── Extract webroot ────────────────────────────────────────────────────────────

ui_print "- Extracting webroot"
unzip -o "$ZIPFILE" "webroot/*" -d "$MODPATH" -x "*.sha256" >&2

# ── Config setup ───────────────────────────────────────────────────────────────

ui_print "- Setting up config directory"
make_dir "$MODULE_CONFIG"

# Write default config.json if not already present
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

# ── Permissions ────────────────────────────────────────────────────────────────

ui_print "- Setting permissions"
set_perm_recursive "$MODPATH/system/bin" 0 0 0755 0755

# ── SoC info (just for display — inherited from Flux) ─────────────────────────

SOC_CODE="0"
[ -f "$FLUX_CONFIG/soc_recognition" ] && SOC_CODE=$(cat "$FLUX_CONFIG/soc_recognition")

case "$SOC_CODE" in
1) ui_print "- Detected MediaTek — thermal tweaks ready" ;;
2) ui_print "- Detected Snapdragon — thermal tweaks ready" ;;
3) ui_print "- Detected Exynos — thermal tweaks ready" ;;
4) ui_print "- Detected Unisoc — thermal tweaks ready" ;;
5) ui_print "- Detected Google Tensor — thermal tweaks ready" ;;
6) ui_print "- Detected Nvidia Tegra — thermal tweaks ready" ;;
7) ui_print "- Detected Kirin — thermal tweaks ready" ;;
0) ui_print "- Unknown SoC — generic thermal tweaks will be applied" ;;
esac

# Easter Egg
case "$((RANDOM % 8 + 1))" in
1) ui_print "- Keeping it cool." ;;
2) ui_print "- Thermal management engaged." ;;
3) ui_print "- No more throttling nightmares." ;;
4) ui_print "- Heat dissipation protocol active." ;;
5) ui_print "- Your device just got a little cooler." ;;
6) ui_print "- Running cool, staying fast." ;;
7) ui_print "- Powered by Flux Thermal." ;;
8) ui_print "- Integrated with Flux Tweaks." ;;
esac
