/* Classes group static methods; unsupported object semantics fail before emission. */
#include "internal.h"

bool gcs_parse_class(gcs_parser* p, const char* space, bool exported) {
    if (!gcs_expect(p, "class")) { return false; }

    const char* name = gcs_identifier(p);
    gcs_class* owner = gcs_allocate(p->compilation, sizeof(*owner));

    if (!owner) { return false; }

    const char* prefix = *space ? gcs_join(p->compilation, space, ".") : "";
    owner->name = prefix ? gcs_join(p->compilation, prefix, name) : NULL;
    owner->space = space;
    owner->exported = exported;
    owner->source = p->source;
    owner->next = p->compilation->classes;
    p->compilation->classes = owner;
    p->owner = owner;

    if (gcs_is(p, ":")) {
        gcs_error(p->compilation, p->source->path, p->token.line, "inheritance and interface implementations are not implemented");

        return false;
    }

    if (!owner->name || !gcs_expect(p, "{")) { return false; }

    while (!gcs_is(p, "}") && p->token.kind != GCS_EOF && !p->compilation->diagnostic[0]) {
        if (!gcs_parse_member(p)) { return false; }
    }

    p->owner = NULL;

    return gcs_expect(p, "}");
}
