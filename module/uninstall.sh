#!/system/bin/sh
#
# Copyright (C) 2024-2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0

rm -rf /data/adb/.config/flux_thermal
rm -f /data/adb/service.d/.flux_thermal_cleanup.sh

need_gone="flux_thermald flux_thermal_profiler flux_thermal_utility"
manager_paths="/data/adb/ap/bin /data/adb/ksu/bin"

for dir in $manager_paths; do
    [ -d "$dir" ] && {
        for bin in $need_gone; do
            rm -f "$dir/$bin"
        done
    }
done
