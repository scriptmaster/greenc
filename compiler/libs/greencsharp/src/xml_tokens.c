/* XML names, whitespace, literal attributes and bounded entity decoding. No DTD expansion. */
#include "internal.h"
#include <ctype.h>

void gcs_xml_space(gcs_xml_parser* p) {
    while (*p->cursor && isspace((unsigned char)*p->cursor)) { ++p->cursor; }
}

const char* gcs_xml_name(gcs_xml_parser* p) {
    const char* start = p->cursor;

    while (isalnum((unsigned char)*p->cursor) || *p->cursor == '_' || *p->cursor == ':' || *p->cursor == '-') { ++p->cursor; }

    if (start == p->cursor) {
        gcs_error(p->compilation, p->path, 0, "expected XML name");

        return "";
    }

    return gcs_copy(p->compilation, start, (size_t)(p->cursor - start));
}

const char* gcs_xml_decode(gcs_xml_parser* p, const char* start, size_t length) {
    char* decoded = gcs_allocate(p->compilation, length + 1);

    if (!decoded) { return ""; }

    size_t output = 0;

    for (size_t i = 0; i < length; ++i) {
        if (start[i] != '&') {
            decoded[output++] = start[i];

            continue;
        }

        const char* entities[] = {"&amp;", "&lt;", "&gt;", "&quot;", "&apos;"};
        const char values[] = {'&', '<', '>', '"', '\''};
        size_t entity = 0;

        while (entity < 5 && (strlen(entities[entity]) > length - i ||
               memcmp(start + i, entities[entity], strlen(entities[entity])))) { ++entity; }

        if (entity == 5) {
            gcs_error(p->compilation, p->path, 0, "unsupported or invalid XML entity");

            return decoded;
        }

        decoded[output++] = values[entity];
        i += strlen(entities[entity]) - 1;
    }

    return decoded;
}

gcs_xml_attribute* gcs_xml_attributes(gcs_xml_parser* p) {
    gcs_xml_attribute* first = NULL;

    for (;;) {
        gcs_xml_space(p);

        if (*p->cursor == '>' || *p->cursor == '/' || p->compilation->diagnostic[0]) { return first; }

        gcs_xml_attribute* attribute = gcs_allocate(p->compilation, sizeof(*attribute));

        if (!attribute) { return first; }

        attribute->name = gcs_xml_name(p);
        gcs_xml_space(p);

        if (*p->cursor != '=') { break; }

        ++p->cursor;
        gcs_xml_space(p);
        char quote = *p->cursor;

        if (quote != '\'' && quote != '"') { break; }

        const char* start = ++p->cursor;

        while (*p->cursor && *p->cursor != quote && *p->cursor != '<') { ++p->cursor; }

        if (*p->cursor != quote) { break; }

        attribute->value = gcs_xml_decode(p, start, (size_t)(p->cursor - start));
        ++p->cursor;

        for (gcs_xml_attribute* existing = first; existing; existing = existing->next) {
            if (attribute->name && !strcmp(existing->name, attribute->name)) {
                gcs_error(p->compilation, p->path, 0, "duplicate XML attribute");
            }
        }

        attribute->next = first;
        first = attribute;
    }

    gcs_error(p->compilation, p->path, 0, "malformed XML attribute");

    return first;
}
