
int greenc_main_test() {
    
    // char* msg = greenc_concat("Version: ", VERSION);
    // // defer msg; // Every * variable is automatically released after their last used in their declared scope.

    // if (msg) {
    //     print(msg);
    // }
    // // free(msg); msg = NULL;
    // release(msg);
    // // release(msg); // No double-free errors with release(msg);

    char* cwd = greenc_getcwd_alloc();
    if (cwd) {
        print("cwd:");
        print(cwd);

        tree(cwd);
    }

    char* content = greenc_read_file("main.c");
    if (content) {
        print("file read ok");
        int len = greenc_strlen(content);
    } else {
        print("file not found");
    }

    free(content); content = NULL;
    // free(content); // test: double-free no errors :)

    char* source = "fn main() { print(\"hi world\"); }";

    // SEARCH: lexer_loop
    greenc_lexer lexer = { source, 0 };
    greenc_keywords keywords;
    greenc_keywords_init(&keywords);

    while (1) {
        greenc_token tok = greenc_next_token(&lexer, &keywords); ///stack
        if (tok.kind == TOK_EOF) break;
        sys_write("Token=", 6);
        sys_write(tok.start, tok.len);
        sys_write("\n", 1);
    }


    // ANCHOR: files::collect
    string_list files = greenc_files_collect_filter(cwd, filter_c_files); ///own
    greenc_string_list_sort(&files);

    for (int i = 0; i < files.count; i++) { // no triple-slash marker required for 0
        char* path = files.items[i]; ///read
        char* src = greenc_read_file(path); ///own
        if (!src) continue;

        print("---- parsing file ----");
        print(path);

        // SEARCH: parse_file(
        parse_file(src, &keywords);

        greenc_main_for_defer:
        greenc_free(src); ///free owned in scope
    }

    greenc_string_list_free(&files);

    print("done.");

greenc_main_defer:
    greenc_free(cwd);

    return 0;
}
