#!/system/bin/sh
#
# Copyright (C) 2024-2026 FebriCahyaa
#
# Licensed under the Apache License, Version 2.0 (the "License")
# ...

MODULE_DIR="/data/adb/modules/flux_thermal"
THIS_SCRIPT="/data/adb/service.d/.flux_thermal_cleanup.sh"

if [ ! -d "$MODULE_DIR/disable" ]; then
    cat "$MODULE_DIR/module.prop.orig" >"$MODULE_DIR/module.prop"
    rm -f "$THIS_SCRIPT"
fi
