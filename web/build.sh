#!/bin/sh
# Build the browser REPL as a single self-contained HTML file.
#
#   sh web/build.sh            -> web/dist/repl.html
#
# Self-contained on purpose: the native build stays a plain CMake project, the
# wasm build is driven from here only. Requirements: emscripten on PATH. The
# output has no external dependencies, so it opens from file:// as well as
# over HTTP. This is the same script CI runs for GitHub Pages.
set -e

cd "$(dirname "$0")/.."

if ! command -v em++ >/dev/null 2>&1; then
  echo "error: em++ not found. Install emscripten (brew install emscripten) first." >&2
  exit 1
fi

OUT=web/dist
mkdir -p "$OUT"

em++ \
  -O2 \
  -std=c++11 \
  -fexceptions \
  -sSINGLE_FILE=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sINITIAL_MEMORY=33554432 \
  -sEXPORTED_FUNCTIONS=_scm_wasm_init,_scm_wasm_eval,_scm_wasm_failed,_scm_wasm_free,_scm_wasm_version,_malloc,_free \
  -sEXPORTED_RUNTIME_METHODS=cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8 \
  -sMODULARIZE=0 \
  -sENVIRONMENT=web,worker \
  -sASSERTIONS=0 \
  -DSCM_WASM=1 \
  --shell-file web/shell.html \
  -o "$OUT/repl.html" \
  web/scmwasm.cpp \
  vm.cpp \
  scmapi.cpp \
  scmbasic.cpp \
  scmmath.cpp \
  scmstring.cpp \
  scmcompiler.cpp \
  scmport.cpp \
  scmvector.cpp \
  scmtable.cpp \
  scminit.cpp

echo "built $OUT/repl.html"
