/*
 ** main.c - 1 function.
 ** main - Pass the application arguments to editor startup.
 ** See also: editor_start.h.
 */
#include "editor_start.h"

// greenc: resource assets/editor.rc
// greenc: prepare g++ -std=c++17 -Os -shared -static explorer/command.cpp -o bin/GreenCExplorer.dll -lshlwapi -lshell32 -lole32 -luuid
// greenc: prepare g++ -std=c++17 -Os -static -municode explorer/pack.cpp -o build/greenc/pack-explorer.exe -lshlwapi -lurlmon -lole32
// greenc: prepare build/greenc/pack-explorer.exe explorer/AppxManifest.xml explorer/logo.png bin/GreenCExplorer.msix.tmp
// greenc: publish bin/GreenCExplorer.msix.tmp|bin/GreenCExplorer.msix
// greenc: link -mwindows -lshell32 -lole32

int main(int argc, char** argv) {
    return editor_start(argc, argv);
}
