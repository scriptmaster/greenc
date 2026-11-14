CC = gcc
LIB = ../../../compiler/libs
CPPFLAGS += -Isrc/app -I$(LIB)
CFLAGS = -Os -std=c17 -Wall -Wextra -Werror
LDLIBS = -lpsapi -lshell32 -lole32 -lcomdlg32 -lopengl32 -lgdi32 -luser32

MODEL = $(wildcard src/app/*/*.c src/app/views/ui/*.c)
GENERATED = $(patsubst src/%.c,build/generated/%.c,$(MODEL))
BACKENDS = $(LIB)/sys/process_stats_win32.c $(LIB)/sys/fs/entries_win32.c $(LIB)/sys/console_win32.c $(LIB)/sys/shell_win32.c $(LIB)/sys/fs/directory_win32.c $(LIB)/sys/fs/write_atomic_win32.c \
           $(LIB)/ui/ui_dialog_win32.c $(LIB)/ui/ui_window_win32.c $(LIB)/ui/ui_gpu_win32.c $(LIB)/ui/ui_renderer_win32.c
HEADERS = $(LIB)/sys/process_stats.h $(LIB)/sys/fs/entries.h $(wildcard src/app/*.h $(LIB)/ui/*.h) $(LIB)/sys/console.h $(LIB)/sys/shell.h $(LIB)/sys/fs/directory.h \
          $(LIB)/sys/fs/write_atomic.h $(LIB)/sys/builtin/types_intrinsic.h

.PHONY: all run test clean
all: editor.exe
bin build:
	mkdir -p $@
bin/greenc.exe: ../../../compiler/main.c $(shell find ../../../compiler -name '*.h') | bin
	$(CC) -Os -std=c17 -DGREENC_MAIN -static-libgcc -ffreestanding -fno-builtin -nostdlib -I$(LIB) $< -o $@ -lshell32 -lkernel32 -luser32 -Wl,-egreenc_start

build/editor_main.c: src/main.c bin/greenc.exe | build
	bin/greenc.exe $< -libc --emit-c -o $@

build/generated/%.c: src/%.c bin/greenc.exe src/app/editor.ownership
	mkdir -p "$(dir $@)"
	bin/greenc.exe $< --hosted-module --contracts src/app/editor.ownership -o $@

build/editor-icon.o: assets/editor.rc assets/greenc.ico | build
	windres -I . -i $< -o $@

bin/GreenCExplorer.dll: explorer/command.cpp | bin
	$(CXX) -std=c++17 -Os -shared -static $< -o $@ -lshlwapi -lshell32 -lole32 -luuid

build/pack-explorer.exe: explorer/pack.cpp | build
	$(CXX) -std=c++17 -Os -static -municode $< -o $@ -lshlwapi -lurlmon -lole32

bin/GreenCExplorer.msix: explorer/AppxManifest.xml explorer/logo.png build/pack-explorer.exe | bin
	build/pack-explorer.exe $< explorer/logo.png $@.tmp
	mv -f $@.tmp $@

EDITOR_BINARY ?= bin/editor.exe
$(EDITOR_BINARY): build/editor_main.c $(GENERATED) $(BACKENDS) $(HEADERS) build/editor-icon.o | bin bin/GreenCExplorer.dll bin/GreenCExplorer.msix
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c %.o,$^) -o $@ -mwindows $(LDLIBS)
editor.exe: bin/editor.exe
	MSYS=winsymlinks:nativestrict ln -sf bin/editor.exe editor.exe
run: editor.exe
	./editor.exe
build/hosted_control.c: ../../../compiler/test/hosted_control.gc bin/greenc.exe | build
	bin/greenc.exe $< --hosted-module -o $@

bin/hosted_control.exe: build/hosted_control.c
	$(CC) $(CFLAGS) $< -o $@

ownership-test: bin/greenc.exe | build
	@for fixture in ../../../compiler/test/ownership_*.gc; do \
	    if bin/greenc.exe "$$fixture" --hosted-module -o build/ownership_rejected.c > bin/ownership_test.log; then echo "unexpected success: $$fixture"; exit 1; fi; \
	    grep -q 'ownership:' bin/ownership_test.log || { cat bin/ownership_test.log; exit 1; }; \
	done

call-contract-test: bin/greenc.exe | build
	@for fixture in ../../../compiler/test/call_fail_*.gc; do \
	    if bin/greenc.exe "$$fixture" --hosted-module --contracts ../../../compiler/test/calls.ownership -o build/call_rejected.c > bin/call_test.log; then echo "unexpected success: $$fixture"; exit 1; fi; \
	    grep -q 'ownership:' bin/call_test.log || { cat bin/call_test.log; exit 1; }; \
	done

build/debug_target.c: test/debug_target.gc bin/greenc.exe | build
	bin/greenc.exe $< --hosted-module -o $@

bin/debug_target.exe: build/debug_target.c
	$(CC) -std=c17 -O0 -g $< -o $@

debug-test: bin/tests.exe bin/debug_target.exe
	./bin/tests.exe --debugger gdb.exe

test: call-contract-test ownership-test bin/tests.exe bin/hosted_control.exe
	./bin/hosted_control.exe
	./bin/tests.exe
bin/tests.exe: test/tests.c $(GENERATED) $(BACKENDS) $(HEADERS) | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDLIBS) -Wl,--wrap=ui_clipboard_write
bin/scroll_benchmark.exe: test/scroll_benchmark.c $(GENERATED) $(BACKENDS) $(HEADERS) | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDLIBS)
clean:
	$(RM) bin/greenc.exe build/editor_main.c editor.exe bin/editor.exe bin/tests.exe bin/scroll_benchmark.exe
