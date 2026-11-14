#!/bin/sh
# Bootstrap the compiler for this Linux/macOS host, independent of the caller's directory.
set -eu
bootstrap_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$bootstrap_root"

case $(uname -s) in
    Linux) bootstrap_dl=-ldl; bootstrap_shared=-shared; bootstrap_extension=so ;;
    Darwin) bootstrap_dl=; bootstrap_shared=-dynamiclib; bootstrap_extension=dylib ;;
    *) printf '%s\n' 'Use build.bat on Windows; this script supports Linux and macOS.' >&2; exit 1 ;;
esac

bootstrap_cc=${CC:-cc}
if ! command -v "$bootstrap_cc" >/dev/null 2>&1; then
    printf '%s\n' 'Install a native C compiler, or set CC to its executable path.' >&2
    exit 1
fi

mkdir -p bin/bootstrap
"$bootstrap_cc" -Os -std=c17 -D_POSIX_C_SOURCE=200809L -DGREENC_MAIN -DGREENC_HOSTED \
    -Icompiler/libs compiler/main.c -o bin/bootstrap/greenc $bootstrap_dl
printf '%s\n' 'int main(void) { return 0; }' > bin/bootstrap/smoke.c
bin/bootstrap/greenc bin/bootstrap/smoke.c --hosted-module -o bin/bootstrap/smoke.generated.c
"$bootstrap_cc" -Os -std=c17 -D_POSIX_C_SOURCE=200809L -DGREENC_HOSTED \
    -Icompiler/libs compiler/test/hosted_runtime.c -o bin/bootstrap/hosted-runtime
bin/bootstrap/hosted-runtime
mkdir -p compiler/libs/greencsharp/bin
"$bootstrap_cc" -Os -std=c17 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
    -DGREENC_CSHARP_BUILD -fPIC -fvisibility=hidden -Icompiler/libs -Icompiler/libs/greencsharp/include \
    "$bootstrap_shared" compiler/libs/greencsharp/src/*.c -o "bin/bootstrap/greencsharp.$bootstrap_extension"
printf '%s\n' 'System.Console.WriteLine(42);' > bin/bootstrap/smoke.cs
export GREENC_CSHARP_CC="$bootstrap_cc"
bin/bootstrap/greenc bin/bootstrap/smoke.cs --emit-c -o bin/bootstrap/csharp-smoke.c
"$bootstrap_cc" -Os -std=c17 -Wall -Wextra -Werror -Icompiler/libs compiler/test/greencsharp_api.c \
    -o bin/bootstrap/greencsharp-api $bootstrap_dl
bin/bootstrap/greencsharp-api "$bootstrap_root/bin/bootstrap/greencsharp.$bootstrap_extension" bin/bootstrap/smoke.cs
mv -f "bin/bootstrap/greencsharp.$bootstrap_extension" "compiler/libs/greencsharp/bin/greencsharp.$bootstrap_extension"
mv -f bin/bootstrap/greenc greenc
printf 'Built %s/greenc\n' "$bootstrap_root"
