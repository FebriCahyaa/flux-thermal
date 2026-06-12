#!/bin/env bash
# shellcheck disable=SC2035

if [ -z "$GITHUB_WORKSPACE" ]; then
    echo "This script should only run on GitHub Actions!" >&2
    exit 1
fi

cd "$GITHUB_WORKSPACE" || {
    echo "Unable to cd to GITHUB_WORKSPACE" >&2
    exit 1
}

# ── Version info ──────────────────────────────────────────────────────────────

version="$(cat version)"
version_code="$(git rev-list HEAD --count)"
release_code="$(git rev-list HEAD --count)-$(git rev-parse --short HEAD)-release"
sed -i "s/version=.*/version=$version ($release_code)/" module/module.prop
sed -i "s/versionCode=.*/versionCode=$version_code/"    module/module.prop

# ── Copy files into module ────────────────────────────────────────────────────

cp -r ./libs                      module/
cp -r ./scripts/*                 module/system/bin/
cp LICENSE                        module/
cp banner.webp                    module/ 2>/dev/null || true

# Remove .sh extension from scripts so they're directly executable
find module/system/bin -maxdepth 1 -type f -name "*.sh" \
    -exec sh -c 'mv -- "$0" "${0%.sh}"' {} \;

# ── Integrity checksums ───────────────────────────────────────────────────────

bash .github/scripts/gen_sha256sum.sh "module"

# ── Zip ───────────────────────────────────────────────────────────────────────

zipName="flux_thermal-$version-$release_code.zip"
echo "zipName=$zipName" >>"$GITHUB_OUTPUT"

cd ./module || {
    echo "Unable to cd to ./module" >&2
    exit 1
}

zip -r9 ../"$zipName" * -x *placeholder* *.map .shellcheckrc
zip -z  ../"$zipName" << EOF
$version-$release_code
Build Date $(date +"%a %b %d %H:%M:%S %Z %Y")
EOF
