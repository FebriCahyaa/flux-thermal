#!/system/bin/sh
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

# shellcheck disable=SC2317,SC3006,SC3018,SC3034,SC3057,SC3037

###################################
# Environment vars (set by daemon)
###################################

# FLUX_THERMAL_SOC          SoC code: 0=unknown 1=MTK 2=QC 3=Exynos 4=Unisoc 5=Tensor 6=Tegra 7=Kirin
# FLUX_THERMAL_IS_GKI       1 = GKI kernel
# FLUX_THERMAL_CHARGING     1 = charging
# FLUX_THERMAL_AGGRESSIVE   1 = aggressive mode
# FLUX_THERMAL_MAX_TEMP_MC  Current max zone temperature in millidegrees C

MODULE_CONFIG="/data/adb/.config/flux_thermal"
FLUX_CONFIG="/data/adb/.config/flux"

SOC="${FLUX_THERMAL_SOC:-0}"
IS_GKI="${FLUX_THERMAL_IS_GKI:-0}"
CHARGING="${FLUX_THERMAL_CHARGING:-0}"
AGGRESSIVE="${FLUX_THERMAL_AGGRESSIVE:-0}"
MAX_TEMP_MC="${FLUX_THERMAL_MAX_TEMP_MC:-0}"

###################################
# Helpers
###################################

apply() {
    [ ! -f "$2" ] && return 1
    chmod 644 "$2" >/dev/null 2>&1
    echo "$1" >"$2" 2>/dev/null
    chmod 444 "$2" >/dev/null 2>&1
}

write() {
    [ ! -f "$2" ] && return 1
    chmod 644 "$2" >/dev/null 2>&1
    echo "$1" >"$2" 2>/dev/null
}

apply_non_gki() {
    [ "$IS_GKI" -eq 1 ] && return 0
    apply "$1" "$2"
}

apply_gki() {
    [ "$IS_GKI" -eq 0 ] && return 0
    apply "$1" "$2"
}

change_cpu_gov() {
    chmod 644 /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null
    chmod 644 /sys/devices/system/cpu/cpufreq/policy*/scaling_governor 2>/dev/null
    echo "$1" | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null 2>&1
    echo "$1" | tee /sys/devices/system/cpu/cpufreq/policy*/scaling_governor >/dev/null 2>&1
}

which_maxfreq() { tr ' ' '\n' <"$1" | sort -nr | head -n 1; }
which_midfreq() {
    total=$(wc -w <"$1")
    mid=$(( (total + 1) / 2 ))
    tr ' ' '\n' <"$1" | grep -v '^[[:space:]]*$' | sort -nr | head -n "$mid" | tail -n 1
}
which_minfreq() {
    tr ' ' '\n' <"$1" | grep -v '^[[:space:]]*$' | sort -n | head -n 1
}

cpufreq_unlock() {
    for path in /sys/devices/system/cpu/cpufreq/policy*; do
        [ -f "$path/scaling_max_freq" ] && {
            max=$(cat "$path/cpuinfo_max_freq" 2>/dev/null)
            min=$(cat "$path/cpuinfo_min_freq" 2>/dev/null)
            write "$max" "$path/scaling_max_freq"
            write "$min" "$path/scaling_min_freq"
        }
    done
}

cpufreq_set_max_scale() {
    # $1 = fraction (e.g. 75 for 75%)
    FRACTION="$1"
    for path in /sys/devices/system/cpu/cpufreq/policy*; do
        [ -f "$path/scaling_available_frequencies" ] && {
            total=$(wc -w <"$path/scaling_available_frequencies")
            # pick OPP at given percentile from the top
            pick=$(( total * (100 - FRACTION) / 100 + 1 ))
            [ "$pick" -lt 1 ] && pick=1
            freq=$(tr ' ' '\n' <"$path/scaling_available_frequencies" | \
                   grep -v '^[[:space:]]*$' | sort -nr | head -n "$pick" | tail -n 1)
            [ -n "$freq" ] && write "$freq" "$path/scaling_max_freq"
        }
    done
}

###################################
# Thermal policy helpers
###################################

# Set kernel thermal policy to step_wise (reactive to trip points)
set_thermal_step_wise() {
    for dir in /sys/class/thermal/thermal_zone*; do
        apply "step_wise" "$dir/policy"
    done
}

# Set kernel thermal policy to power_allocator (proactive budget-based)
set_thermal_power_allocator() {
    for dir in /sys/class/thermal/thermal_zone*; do
        apply "power_allocator" "$dir/policy" 2>/dev/null || true
    done
}

# Disable kernel-level thermal throttling entirely (use with care)
set_thermal_user_space() {
    for dir in /sys/class/thermal/thermal_zone*; do
        apply "user_space" "$dir/policy" 2>/dev/null || true
    done
}

###################################
# GPU frequency helpers
###################################

gpu_set_max_freq() {
    # Snapdragon
    for f in /sys/class/kgsl/kgsl-3d0/devfreq/max_freq \
             /sys/class/kgsl/kgsl-3d0/max_gpuclk; do
        [ -f "$f" ] && write "$1" "$f"
    done

    # Mali (Exynos / MediaTek)
    for f in /sys/class/misc/mali0/device/dvfs_max_lock \
             /sys/kernel/gpu/gpu_max_clock; do
        [ -f "$f" ] && write "$1" "$f"
    done
}

gpu_unlock_freq() {
    # Snapdragon
    [ -f /sys/class/kgsl/kgsl-3d0/devfreq/max_freq ] && {
        max=$(cat /sys/class/kgsl/kgsl-3d0/devfreq/available_frequencies 2>/dev/null | \
              tr ' ' '\n' | sort -nr | head -n 1)
        [ -n "$max" ] && write "$max" /sys/class/kgsl/kgsl-3d0/devfreq/max_freq
    }

    # Mali
    [ -f /sys/class/misc/mali0/device/dvfs_max_lock ] && \
        write 0 /sys/class/misc/mali0/device/dvfs_max_lock
}

###################################
# SoC-specific: MediaTek
###################################

mtk_set_thermal_policy() {
    # PICACHU / MET thermal
    [ -f /proc/cooler/tzref ] && write "$1" /proc/cooler/tzref

    # PPM (Performance Policy Manager) — limit cluster max
    if [ "$1" = "limit_hot" ]; then
        cluster=-1
        for path in /sys/devices/system/cpu/cpufreq/policy*; do
            ((cluster++))
            midfreq=$(which_midfreq "$path/scaling_available_frequencies")
            write "$cluster $midfreq" /proc/ppm/policy/hard_userlimit_max_cpu_freq 2>/dev/null
        done
    elif [ "$1" = "limit_critical" ]; then
        cluster=-1
        for path in /sys/devices/system/cpu/cpufreq/policy*; do
            ((cluster++))
            minfreq=$(which_minfreq "$path/scaling_available_frequencies")
            write "$cluster $minfreq" /proc/ppm/policy/hard_userlimit_max_cpu_freq 2>/dev/null
        done
    else
        # Unlock
        cluster=-1
        for path in /sys/devices/system/cpu/cpufreq/policy*; do
            ((cluster++))
            maxfreq=$(cat "$path/cpuinfo_max_freq" 2>/dev/null)
            write "$cluster $maxfreq" /proc/ppm/policy/hard_userlimit_max_cpu_freq 2>/dev/null
            write "$cluster 0" /proc/ppm/policy/hard_userlimit_min_cpu_freq 2>/dev/null
        done
    fi
}

mtk_gpu_thermal() {
    # EARA GPU
    [ -f /sys/kernel/ged/hal/gpu_dvfs_enable ] && \
        write "$1" /sys/kernel/ged/hal/gpu_dvfs_enable
    [ -f /sys/kernel/ged/hal/is_GED_KPI_enabled ] && \
        write "$1" /sys/kernel/ged/hal/is_GED_KPI_enabled
}

###################################
# SoC-specific: Snapdragon
###################################

qcom_set_thermal_policy() {
    # MSM thermal: throttle via cooling devices
    for f in /sys/class/thermal/cooling_device*/cur_state; do
        [ "$1" = "unlock" ] && write 0 "$f"
    done

    # DCVS bandwidth
    if [ "$1" = "limit_hot" ] || [ "$1" = "limit_critical" ]; then
        for f in /sys/class/devfreq/soc:qcom,cpu-cpu-ddr-bw/max_freq; do
            [ -f "$f" ] && {
                min=$(cat "${f%max_freq}min_freq" 2>/dev/null)
                [ -n "$min" ] && write "$min" "$f"
            }
        done
    else
        for f in /sys/class/devfreq/soc:qcom,cpu-cpu-ddr-bw/max_freq; do
            [ -f "$f" ] && {
                max=$(cat "${f%max_freq}available_frequencies" 2>/dev/null | \
                      tr ' ' '\n' | sort -nr | head -n 1)
                [ -n "$max" ] && write "$max" "$f"
            }
        done
    fi
}

###################################
# SoC-specific: Exynos
###################################

exynos_set_thermal_policy() {
    # Exynos TherMAL IPC
    [ -f /sys/class/thermal/thermal_zone0/emul_temp ] && \
        write 0 /sys/class/thermal/thermal_zone0/emul_temp
}

###################################
# SoC-specific: Google Tensor
###################################

tensor_set_thermal_policy() {
    # BCL (Battery Current Limiter) — relax on cool, tighten on hot
    if [ "$1" = "limit_hot" ] || [ "$1" = "limit_critical" ]; then
        apply 1 /sys/devices/platform/google,bcl/enabled 2>/dev/null || true
    else
        apply 0 /sys/devices/platform/google,bcl/enabled 2>/dev/null || true
    fi
}

###################################
# Profile functions
###################################

profile_normal() {
    set_thermal_step_wise

    # Unlock all CPU/GPU frequency limits
    cpufreq_unlock
    gpu_unlock_freq

    # Restore default governor (read from Flux's cached value)
    default_gov="schedutil"
    [ -f "$FLUX_CONFIG/default_cpu_gov" ] && default_gov=$(cat "$FLUX_CONFIG/default_cpu_gov")
    change_cpu_gov "$default_gov"

    # VM
    write 120 /proc/sys/vm/vfs_cache_pressure

    case $SOC in
    1) mtk_set_thermal_policy unlock ; mtk_gpu_thermal 1 ;;
    2) qcom_set_thermal_policy unlock ;;
    3) exynos_set_thermal_policy unlock ;;
    5) tensor_set_thermal_policy unlock ;;
    esac
}

profile_cool() {
    set_thermal_step_wise

    # Unlock CPU but cap GPU to 80%
    cpufreq_unlock

    default_gov="schedutil"
    [ -f "$FLUX_CONFIG/default_cpu_gov" ] && default_gov=$(cat "$FLUX_CONFIG/default_cpu_gov")
    change_cpu_gov "$default_gov"

    # Light VM pressure
    write 100 /proc/sys/vm/vfs_cache_pressure

    gpu_unlock_freq  # Still full GPU at cool

    case $SOC in
    1) mtk_set_thermal_policy unlock ; mtk_gpu_thermal 1 ;;
    2) qcom_set_thermal_policy unlock ;;
    3) exynos_set_thermal_policy unlock ;;
    5) tensor_set_thermal_policy unlock ;;
    esac
}

profile_warm() {
    set_thermal_step_wise

    # Cap CPU to ~75% of max OPP
    cpufreq_set_max_scale 75

    change_cpu_gov "schedutil"

    # Cap GPU moderately
    # (actual number varies by SoC; per-SoC blocks below override if needed)
    gpu_unlock_freq

    write 100 /proc/sys/vm/vfs_cache_pressure

    case $SOC in
    1) mtk_set_thermal_policy limit_warm ; mtk_gpu_thermal 1 ;;
    2) qcom_set_thermal_policy limit_warm ;;
    3) exynos_set_thermal_policy limit_warm ;;
    5) tensor_set_thermal_policy unlock ;;
    esac

    # I/O — slightly reduced
    for dir in /sys/block/mmcblk0 /sys/block/mmcblk1 /sys/block/sd*; do
        write 64 "$dir/queue/read_ahead_kb"
        write 32 "$dir/queue/nr_requests"
    done &
}

profile_hot() {
    set_thermal_step_wise

    # Cap CPU to ~50% OPP
    cpufreq_set_max_scale 50

    change_cpu_gov "powersave"

    write 80 /proc/sys/vm/vfs_cache_pressure

    # GPU — limit to 50%
    case $SOC in
    1)
        mtk_set_thermal_policy limit_hot
        mtk_gpu_thermal 0
        # MediaTek: halve GPU via ged
        [ -f /sys/kernel/ged/hal/gpu_dvfs_enable ] && write 0 /sys/kernel/ged/hal/gpu_dvfs_enable
        ;;
    2)
        qcom_set_thermal_policy limit_hot
        [ -f /sys/class/kgsl/kgsl-3d0/devfreq/available_frequencies ] && {
            mid=$(cat /sys/class/kgsl/kgsl-3d0/devfreq/available_frequencies | \
                  tr ' ' '\n' | sort -nr | awk 'NR==int(NF/2)+1')
            [ -n "$mid" ] && write "$mid" /sys/class/kgsl/kgsl-3d0/devfreq/max_freq
        }
        ;;
    3) exynos_set_thermal_policy limit_hot ;;
    5) tensor_set_thermal_policy limit_hot ;;
    esac

    for dir in /sys/block/mmcblk0 /sys/block/mmcblk1 /sys/block/sd*; do
        write 128 "$dir/queue/read_ahead_kb"
        write 64  "$dir/queue/nr_requests"
    done &
}

profile_critical() {
    # Emergency thermal mode — bring everything to the floor

    set_thermal_power_allocator

    # CPU → absolute minimum OPP
    cpufreq_set_max_scale 20
    change_cpu_gov "powersave"

    # GPU → minimum
    case $SOC in
    1)
        mtk_set_thermal_policy limit_critical
        mtk_gpu_thermal 0
        ;;
    2)
        qcom_set_thermal_policy limit_hot
        [ -f /sys/class/kgsl/kgsl-3d0/devfreq/available_frequencies ] && {
            min=$(cat /sys/class/kgsl/kgsl-3d0/devfreq/available_frequencies | \
                  tr ' ' '\n' | sort -n | head -n 1)
            [ -n "$min" ] && write "$min" /sys/class/kgsl/kgsl-3d0/devfreq/max_freq
        }
        ;;
    3) exynos_set_thermal_policy limit_critical ;;
    5) tensor_set_thermal_policy limit_hot ;;
    esac

    write 60 /proc/sys/vm/vfs_cache_pressure

    # Drop caches to reduce memory bus heat
    echo 3 >/proc/sys/vm/drop_caches

    for dir in /sys/block/mmcblk0 /sys/block/mmcblk1 /sys/block/sd*; do
        write 128 "$dir/queue/read_ahead_kb"
        write 64  "$dir/queue/nr_requests"
    done &
}

###################################
# Main dispatch
###################################

case "$1" in
"normal")   profile_normal ;;
"cool")     profile_cool ;;
"warm")     profile_warm ;;
"hot")      profile_hot ;;
"critical") profile_critical ;;
*)
    echo "Usage: flux_thermal_profiler {normal|cool|warm|hot|critical}" >&2
    exit 1
    ;;
esac

wait
exit 0
