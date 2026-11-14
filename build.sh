#!/bin/sh
# Bootstrap the compiler for this Linux/macOS host, independent of the caller's directory.
set -eu
bootstrap_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$bootstrap_root"

case $(uname -s) in
    Linux|Darwin) ;;
    *) printf '%s\n' 'Use build.bat on Windows; this script supports Linux and macOS.' >&2; exit 1 ;;
esac

bootstrap_cc=${CC:-cc}
if ! command -v "$bootstrap_cc" >/dev/null 2>&1; then
    printf '%s\n' 'Install a native C compiler, or set CC to its executable path.' >&2
    exit 1
fi

mkdir -p bin/bootstrap
"$bootstrap_cc" -Os -std=c17 -D_POSIX_C_SOURCE=200809L -DGREENC_MAIN -DGREENC_HOSTED \
    -Icompiler/libs compiler/main.c -o bin/bootstrap/greenc
printf '%s\n' 'int main(void) { return 0; }' > bin/bootstrap/smoke.c
bin/bootstrap/greenc bin/bootstrap/smoke.c --hosted-module -o bin/bootstrap/smoke.generated.c
"$bootstrap_cc" -Os -std=c17 -D_POSIX_C_SOURCE=200809L -DGREENC_HOSTED \
    -Icompiler/libs compiler/test/hosted_runtime.c -o bin/bootstrap/hosted-runtime
bin/bootstrap/hosted-runtime
mv -f bin/bootstrap/greenc greenc
printf 'Built %s/greenc\n' "$bootstrap_root"
