/*
 ** csharp_build.h - 1 function.
 ** greenc_csharp_build - Delegate native project generation, archives and linking to the loaded frontend.
 ** See also: csharp_delegate.h, libs/greencsharp/include/greencsharp.h.
 */
#pragma once

static int greenc_csharp_build(const greencsharp_api* api, const char* input, const char* output,
                               const char* configuration, u32 mode) {
    greencsharp_request request = {.size = sizeof(request), .version = GREENC_CSHARP_ABI,
        .input_path = input, .output_path = output, .configuration = configuration, .mode = mode};
    void* compilation = api->create(&request);
    const greencsharp_result* result = compilation ? api->result(compilation) : NULL;
    bool valid_result = result && result->size == sizeof(*result);
    int status = valid_result && !result->status ? 0 : 1;

    if (status) {
        print(valid_result && result->diagnostic && result->diagnostic[0] ?
              result->diagnostic : "greencsharp: invalid or failed compilation result");
    }

    if (compilation) { api->destroy(compilation); }

    return status;
}
