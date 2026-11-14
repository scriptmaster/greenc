/* Strict XML element tree for the supported declarative project subset. */
#include "internal.h"
#include <ctype.h>

static bool gcs_xml_misc(gcs_xml_parser* p) {
    gcs_xml_space(p);
    const char* end = NULL;
    size_t delimiter = 0;

    if (!strncmp(p->cursor, "<!--", 4)) {
        end = strstr(p->cursor + 4, "-->");
        delimiter = 3;
    } else if (!strncmp(p->cursor, "<?xml ", 6)) {
        end = strstr(p->cursor + 6, "?>");
        delimiter = 2;
    } else {
        return false;
    }

    if (!end) {
        gcs_error(p->compilation, p->path, 0, "unterminated XML comment or declaration");

        return false;
    }

    p->cursor = end + delimiter;

    return true;
}

static gcs_xml_node* gcs_xml_element(gcs_xml_parser* p, size_t depth) {
    if (depth > 32 || ++p->nodes > 10000 || *p->cursor != '<') {
        gcs_error(p->compilation, p->path, 0, "invalid or excessively nested XML");

        return NULL;
    }

    ++p->cursor;
    gcs_xml_node* node = gcs_allocate(p->compilation, sizeof(*node));

    if (!node) { return NULL; }

    node->name = gcs_xml_name(p);
    node->attributes = gcs_xml_attributes(p);
    node->text = "";

    if (!node->name || p->compilation->diagnostic[0]) { return node; }

    if (!strncmp(p->cursor, "/>", 2)) {
        p->cursor += 2;

        return node;
    }

    if (*p->cursor != '>') {
        gcs_error(p->compilation, p->path, 0, "expected XML tag terminator");

        return node;
    }

    ++p->cursor;
    gcs_xml_node** insertion = &node->children;

    while (*p->cursor && !p->compilation->diagnostic[0]) {
        if (gcs_xml_misc(p)) { continue; }
        if (!strncmp(p->cursor, "</", 2)) { break; }

        if (*p->cursor == '<') {
            *insertion = gcs_xml_element(p, depth + 1);

            if (!*insertion) { return node; }

            insertion = &(*insertion)->next;
        } else {
            const char* start = p->cursor;

            while (*p->cursor && *p->cursor != '<') { ++p->cursor; }

            const char* end = p->cursor;

            while (start < end && isspace((unsigned char)*start)) { ++start; }
            while (end > start && isspace((unsigned char)end[-1])) { --end; }

            if (end > start) {
                if (node->text[0]) { gcs_error(p->compilation, p->path, 0, "mixed XML text is unsupported"); }

                node->text = gcs_xml_decode(p, start, (size_t)(end - start));
            }
        }
    }

    if (strncmp(p->cursor, "</", 2)) {
        gcs_error(p->compilation, p->path, 0, "missing XML closing element");

        return node;
    }

    p->cursor += 2;
    const char* closing = gcs_xml_name(p);
    gcs_xml_space(p);

    if (!closing || strcmp(closing, node->name) || *p->cursor != '>') {
        gcs_error(p->compilation, p->path, 0, "mismatched XML closing element");
    } else {
        ++p->cursor;
    }

    return node;
}

gcs_xml_node* gcs_xml_document(gcs_compilation* c, const char* path) {
    char* text = gcs_read(c, path);

    if (!text) { return NULL; }

    if (!strncmp(text, "\xef\xbb\xbf", 3)) { text += 3; }

    gcs_xml_parser parser = {.compilation = c, .path = path, .cursor = text};

    while (gcs_xml_misc(&parser)) { }

    gcs_xml_node* root = gcs_xml_element(&parser, 0);

    while (gcs_xml_misc(&parser)) { }

    gcs_xml_space(&parser);

    if (*parser.cursor) { gcs_error(c, path, 0, "unexpected text after XML root"); }

    return c->diagnostic[0] ? NULL : root;
}

const char* gcs_xml_value(const gcs_xml_node* node, const char* name) {
    for (gcs_xml_attribute* attribute = node->attributes; attribute; attribute = attribute->next) {
        if (!strcmp(attribute->name, name)) { return attribute->value; }
    }

    return NULL;
}
