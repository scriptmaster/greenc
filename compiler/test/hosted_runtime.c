/* Hosted POSIX bootstrap checks. Run from the repository root. */
#include <stdbool.h>
#include "../libs/libgreenc.h"
#include "../libs/sys/exec.h"

static bool check_file_lifetime(void) {
    const char* path = "bin/bootstrap/runtime.txt";

    if (greenc_write_file(path, "hosted runtime") != 0) { return false; }

    char* content = greenc_read_file(path);
    bool correct = content && greenc_streq(content, "hosted runtime");
    greenc_free(content);

    char* directory = greenc_getcwd_alloc();
    correct = correct && directory && directory[0];
    greenc_free(directory);

    greenc_dirent entries[16];
    correct = correct && greenc_listdir("bin/bootstrap", entries, 16) > 0;

    return correct;
}

static bool check_process_results(void) {
    return greenc_exec_wait("sh -c 'exit 7'") == 7 &&
           greenc_exec_wait("sh -c \"exit 9\"") == 9 &&
           greenc_exec_wait("greenc_nonexistent_tool_93874") == -1 &&
           greenc_exec_wait("sh -c \"unclosed") == -1;
}

int main(void) {
    return check_file_lifetime() && check_process_results() ? 0 : 1;
}
