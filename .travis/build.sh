#!/bin/sh -e

TMP="$1"
OUT="$2"

sed -e "s/\"\(\.5\)\"/\"\1-${SHORT_ID}\"/;" src/version.h > src/version.h.tmp && mv src/version.h.tmp src/version.h
VERSION="$(grep QED_VERSION src/version.h | sed -e 's/^.*\"v\(.*\)\".*/\1/;')"."$(grep QED_REVISION src/version.h | sed -e 's/^.*\"\.\(.*\)\".*/\1/;')"
make

mkdir -p "${TMP}/qed"

make install installdir="${TMP}/qed"

mkdir -p "${OUT}" && cd "${TMP}" && zip -r -9 "${OUT}/${PROJECT}-${VERSION}.zip" *
