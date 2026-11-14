/* Versioned plugin entry point; parsing and all owned state remain inside this library. */
#include "internal.h"

static void* gcs_create(const greencsharp_request* request) {
    if (!request || request->size != sizeof(*request) || request->version != GREENC_CSHARP_ABI ||
        !request->input_path || request->mode > GCS_CLEAN) { return NULL; }

    gcs_compilation* c = calloc(1, sizeof(*c));

    if (!c) { return NULL; }

    c->result.size = sizeof(c->result);
    c->result.status = 1;
    c->result.diagnostic = c->diagnostic;
    if (request->mode != GCS_ANALYZE) {
        bool built = gcs_native_build(c, request);
        c->result.status = built ? 0 : 1;

        if (!built && !c->diagnostic[0]) { gcs_error(c, request->input_path, 0, "native project build failed"); }

        return c;
    }

    bool success = gcs_load_input(c, request->input_path);

    for (gcs_source* source = c->sources; source && success; source = source->next) {
        success = gcs_parse_source(c, source);
    }

    success = success && gcs_check(c) && gcs_prepare_symbols(c) && gcs_emit(c) && gcs_emit_metadata(c);

    if (!success && !c->diagnostic[0]) { gcs_error(c, request->input_path, 0, "invalid or unsupported input"); }

    c->result.status = success ? 0 : 1;
    c->result.program = c->program.data;
    c->result.ast = c->ast.data;
    c->result.dependencies = c->dependencies.data;

    return c;
}

static const greencsharp_result* gcs_result(void* compilation) {
    return compilation ? &((gcs_compilation*)compilation)->result : NULL;
}

static void gcs_destroy(void* compilation) {
    if (compilation) { gcs_release(compilation); }
}

GCS_EXPORT const greencsharp_api* greencsharp_get_api(u32 version) {
    static const greencsharp_api api = {sizeof(greencsharp_api), GREENC_CSHARP_ABI, gcs_create, gcs_result, gcs_destroy};

    return version == GREENC_CSHARP_ABI ? &api : NULL;
}
