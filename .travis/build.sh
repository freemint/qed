#!/bin/sh -e

TMP="$1"
OUT="$2"

sed -e "s/\"\(\.5\)\"/\"\1-${SHORT_ID}\"/;" src/version.h > src/version.h.tmp && mv src/version.h.tmp src/version.h
VERSION="$(grep QED_VERSION src/version.h | sed -e 's/^.*\"v\(.*\)\".*/\1/;')"."$(grep QED_REVISION src/version.h | sed -e 's/^.*\"\.\(.*\)\".*/\1/;')"
make

# TODO: generate HYP
mkdir -p "${TMP}/qed"
cp "src/qed_new.app" "${TMP}/qed/qed.app"
cp "en/qed_en.rsc" "${TMP}/qed/qed.rsc"
cp "en/readme.txt" "${TMP}/qed"
cp "en/readme.old" "${TMP}/qed"
cp -r "en/syntax" "${TMP}/qed"
cp "dist/qed.cfg" "${TMP}/qed"
cp "dist/icons.rsc" "${TMP}/qed"
cp -r "dist/kurzel" "${TMP}/qed"
cp -r "dist/treiber" "${TMP}/qed"

# TODO: generate HYP
mkdir -p "${TMP}/qed/de"
cp "src/qed.rsc" "${TMP}/qed/de"
cp -r "tools/syntax" "${TMP}/qed/de"
cp "dist/liesmich.txt" "${TMP}/qed/de"
cp "dist/liesmich.old" "${TMP}/qed/de"

find "${TMP}" -type f -perm -a=x -exec m68k-atari-mint-strip -s {} \;
mkdir -p "${OUT}" && cd "${TMP}" && zip -r -9 "${OUT}/${PROJECT}-${VERSION}.zip" *
