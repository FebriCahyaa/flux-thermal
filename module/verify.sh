#!/bin/sh
# verify.sh — sourced by customize.sh during installation
# Verifies SHA256 checksums of critical module files to detect corruption.
#
# Copyright (C) 2024-2026 FebriCahyaa
# Licensed under the Apache License, Version 2.0

ui_print "- Verifying module integrity..."

_verify_file() {
    local file="$1"
    local sha256_file="${file}.sha256"

    # Skip if no checksum file (optional files)
    [ ! -f "$sha256_file" ] && return 0

    expected=$(cat "$sha256_file")
    actual=$(sha256sum "$file" | awk '{print $1}')

    if [ "$expected" != "$actual" ]; then
        ui_print "  ! Checksum mismatch: $file"
        ui_print "    expected: $expected"
        ui_print "    actual:   $actual"
        return 1
    fi

    return 0
}

_integrity_failed() {
    ui_print "*********************************************************"
    ui_print "! Integrity check failed!"
    ui_print "! One or more module files are corrupted."
    ui_print "! Please re-download the module."
    abort "*********************************************************"
}

# Verify critical files extracted from the zip
for _critical_file in \
    "$MODPATH/module.prop" \
    "$MODPATH/service.sh" \
    "$MODPATH/customize.sh"; do
    if [ -f "${_critical_file}.sha256" ]; then
        _verify_file "$_critical_file" || _integrity_failed
    fi
done

ui_print "  -> Integrity check: OK"
