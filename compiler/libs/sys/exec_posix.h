/* Hosted POSIX tool execution. Quotes and backslash escapes are decoded without a shell. */
#pragma once
#include <stdbool.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>

extern char** environ;

static bool greenc_exec_arguments(char* text, char** arguments) {
    char* read = text;
    char* write = text;
    size_t count = 0;

    while (*read) {
        while (*read && (unsigned char)*read <= ' ') { ++read; }

        if (!*read) { break; }

        arguments[count++] = write;
        char quote = 0;

        while (*read && (quote || (unsigned char)*read > ' ')) {
            char character = *read++;

            if (character == '\\' && quote != '\'') {
                if (!*read) { return false; }

                *write++ = *read++;
            } else if ((character == '\'' || character == '"') && (!quote || quote == character)) {
                quote = quote ? 0 : character;
            } else {
                *write++ = character;
            }
        }

        if (quote) { return false; }

        while (*read && (unsigned char)*read <= ' ') { ++read; }

        *write++ = 0;
    }

    arguments[count] = NULL;

    return count && arguments[0][0];
}

static int greenc_exec_wait(const char* command) {
    if (!command) { return -1; }

    size_t length = greenc_strlen(command);
    char* text = greenc_copy(command);
    char** arguments = length < ((size_t)-1) / sizeof(char*) - 1 ?
                       (char**)greenc_malloc((length + 1) * sizeof(char*)) : NULL;
    pid_t child;
    bool started = text && arguments && greenc_exec_arguments(text, arguments) &&
                   posix_spawnp(&child, arguments[0], NULL, NULL, arguments, environ) == 0;
    greenc_free(arguments);
    greenc_free(text);

    if (!started) { return -1; }

    int status;
    pid_t waited;

    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);

    if (waited < 0) { return -1; }
    if (WIFEXITED(status)) { return WEXITSTATUS(status); }

    return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
}
