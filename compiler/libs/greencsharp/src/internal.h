/* Native compiler model and internal phase contracts. Ownership belongs to gcs_compilation. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include "greencsharp.h"

typedef struct gcs_allocation gcs_allocation;
typedef struct gcs_project gcs_project;
typedef struct gcs_source gcs_source;
typedef struct gcs_method gcs_method;
typedef struct gcs_class gcs_class;
typedef struct gcs_node gcs_node;
typedef struct gcs_variable gcs_variable;
typedef struct gcs_scope gcs_scope;
typedef struct gcs_reference gcs_reference;
typedef struct gcs_using gcs_using;
typedef struct gcs_object gcs_object;
struct gcs_object { const char* path; gcs_object* next; };
typedef struct gcs_xml_attribute gcs_xml_attribute;
typedef struct gcs_xml_node gcs_xml_node;
struct gcs_xml_attribute { const char* name; const char* value; gcs_xml_attribute* next; };
struct gcs_xml_node {
    const char* name;
    const char* text;
    gcs_xml_attribute* attributes;
    gcs_xml_node* children;
    gcs_xml_node* next;
};

typedef enum { GCS_VOID, GCS_INT, GCS_BOOL, GCS_STRING, GCS_ARGS, GCS_INFER, GCS_INVALID } gcs_type;
typedef enum { GCS_EOF, GCS_IDENTIFIER, GCS_NUMBER, GCS_LITERAL, GCS_PUNCTUATION } gcs_token_kind;
typedef struct { gcs_token_kind kind; const char* text; size_t length; size_t line; } gcs_token;
typedef struct { char* data; size_t length; size_t capacity; } gcs_buffer;

typedef enum {
    GCS_INTEGER, GCS_BOOLEAN, GCS_TEXT, GCS_NAME, GCS_CALL, GCS_UNARY, GCS_BINARY, GCS_ASSIGN,
    GCS_BLOCK, GCS_DECLARE, GCS_EXPRESSION, GCS_RETURN, GCS_IF, GCS_WHILE, GCS_FOR, GCS_BREAK, GCS_CONTINUE,
    GCS_LENGTH, GCS_INDEX, GCS_UPDATE
} gcs_node_kind;

struct gcs_node {
    gcs_node_kind kind;
    gcs_type type;
    gcs_token token;
    const char* name;
    gcs_node* first;
    gcs_node* second;
    gcs_node* third;
    gcs_node* fourth;
    gcs_node* next;
    gcs_variable* variable;
    gcs_method* method;
    bool builtin;
    bool postfix;
    bool constant;
    size_t emitted_id;
    i32 integer;
    size_t height;
};

struct gcs_variable {
    const char* name;
    gcs_type type;
    size_t id;
    gcs_variable* next;
};
struct gcs_scope { gcs_scope* parent; gcs_variable* variables; };
struct gcs_using { const char* name; gcs_using* next; };
struct gcs_reference { gcs_project* project; gcs_reference* next; };
struct gcs_project {
    size_t id;
    const char* path;
    const char* directory;
    gcs_reference* references;
    gcs_project* next;
    bool visiting;
    bool loaded;
    bool executable;
    bool default_sources;
    bool implicit_usings;
    const char* assembly;
    const char* object_directory;
    const char* output_directory;
    const char* output;
    const char* contract;
    const char* public_header;
    u32 build_state;
};
struct gcs_source { const char* path; char* text; gcs_project* project; gcs_using* imports; gcs_source* next; bool imported; };
struct gcs_class {
    const char* name;
    const char* space;
    bool exported;
    gcs_source* source;
    gcs_class* next;
};
struct gcs_method {
    const char* name;
    gcs_class* owner;
    gcs_source* source;
    gcs_type type;
    gcs_variable* parameters;
    gcs_node* body;
    size_t id;
    size_t line;
    u32 access;
    bool top_level;
    const char* symbol;
    size_t next_variable;
    bool is_static;
    gcs_method* next;
};

typedef struct {
    gcs_allocation* allocations;
    gcs_project* projects;
    gcs_source* sources;
    gcs_class* classes;
    gcs_method* methods;
    gcs_method* entry;
    gcs_method* emitting_method;
    size_t source_count;
    size_t project_count;
    size_t node_count;
    size_t method_count;
    size_t variable_count;
    size_t temporary_count;
    char diagnostic[2048];
    gcs_buffer program;
    gcs_buffer ast;
    gcs_buffer dependencies;
    greencsharp_result result;
    const char* root_path;
    const char* target;
    const char* configuration;
    const char* compiler;
    const char* archiver;
    const char* toolchain;
    bool planning;
    bool importing;
    u32 build_mode;
    gcs_object* objects;
    gcs_buffer manifest;
} gcs_compilation;

typedef struct { gcs_compilation* compilation; const char* path; const char* cursor; size_t nodes; } gcs_xml_parser;

typedef struct {
    gcs_compilation* compilation;
    gcs_source* source;
    const char* cursor;
    size_t line;
    size_t depth;
    gcs_token token;
    gcs_class* owner;
} gcs_parser;

void gcs_error(gcs_compilation* c, const char* path, size_t line, const char* format, ...);
void* gcs_allocate(gcs_compilation* c, size_t size);
char* gcs_copy(gcs_compilation* c, const char* text, size_t length);
char* gcs_join(gcs_compilation* c, const char* left, const char* right);
void gcs_release(gcs_compilation* c);
bool gcs_append(gcs_compilation* c, gcs_buffer* buffer, const char* format, ...);
bool gcs_json(gcs_compilation* c, gcs_buffer* buffer, const char* text);
char* gcs_absolute(gcs_compilation* c, const char* path);
const char* gcs_directory(gcs_compilation* c, const char* path);
char* gcs_read(gcs_compilation* c, const char* path);
bool gcs_suffix(const char* path, const char* suffix);
i32 gcs_path_compare(const char* left, const char* right);
bool gcs_path_suffix(const char* path, const char* suffix);
char* gcs_resolve_path(gcs_compilation* c, const char* directory, const char* path);
bool gcs_scan_sources(gcs_compilation* c, gcs_project* project, const char* directory, size_t depth);
bool gcs_add_source(gcs_compilation* c, gcs_project* project, const char* path);
gcs_project* gcs_load_project(gcs_compilation* c, const char* path, size_t depth);
bool gcs_load_input(gcs_compilation* c, const char* path);
bool gcs_read_project(gcs_compilation* c, gcs_project* project, size_t depth);
bool gcs_read_solution(gcs_compilation* c, const char* path);
void gcs_xml_space(gcs_xml_parser* parser);
const char* gcs_xml_name(gcs_xml_parser* parser);
const char* gcs_xml_decode(gcs_xml_parser* parser, const char* start, size_t length);
gcs_xml_attribute* gcs_xml_attributes(gcs_xml_parser* parser);
gcs_xml_node* gcs_xml_document(gcs_compilation* c, const char* path);
const char* gcs_xml_value(const gcs_xml_node* node, const char* name);
void gcs_next(gcs_parser* parser);
bool gcs_is(const gcs_parser* parser, const char* text);
bool gcs_take(gcs_parser* parser, const char* text);
bool gcs_expect(gcs_parser* parser, const char* text);
const char* gcs_identifier(gcs_parser* parser);
gcs_type gcs_parse_type(gcs_parser* parser, bool infer);
gcs_node* gcs_new_node(gcs_parser* parser, gcs_node_kind kind);
gcs_node* gcs_expression(gcs_parser* parser, size_t precedence);
gcs_node* gcs_statement(gcs_parser* parser);
bool gcs_parse_source(gcs_compilation* c, gcs_source* source);
bool gcs_parse_classes(gcs_parser* parser, const char* space, bool nested);
bool gcs_parse_class(gcs_parser* parser, const char* space, bool exported);
bool gcs_check(gcs_compilation* c);
bool gcs_check_constant(gcs_compilation* c, gcs_method* method, gcs_node* node);
gcs_type gcs_check_expression(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node);
bool gcs_check_statement(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_node* node, size_t loops);
gcs_variable* gcs_find_variable(gcs_scope* scope, const char* name);
bool gcs_declare_variable(gcs_compilation* c, gcs_method* method, gcs_scope* scope, gcs_variable* variable, size_t line);
gcs_method* gcs_resolve_method(gcs_compilation* c, gcs_method* caller, const char* name, size_t line);
bool gcs_emit(gcs_compilation* c);
void gcs_emit_runtime(gcs_compilation* c);
void gcs_emit_literal(gcs_compilation* c, const gcs_node* node, size_t id);
size_t gcs_emit_expression(gcs_compilation* c, gcs_node* node);
void gcs_emit_statement(gcs_compilation* c, gcs_node* node);
const char* gcs_type_name(gcs_type type);
bool gcs_emit_metadata(gcs_compilation* c);
u64 gcs_hash_bytes(u64 hash, const void* data, size_t size);
const char* gcs_symbol(gcs_compilation* c, const char* domain, const char* name);
bool gcs_prepare_symbols(gcs_compilation* c);
const char* gcs_assembly_name(gcs_compilation* c, const char* path);
bool gcs_native_build(gcs_compilation* c, const greencsharp_request* request);
bool gcs_make_directories(gcs_compilation* c, const char* path);
bool gcs_write_text(gcs_compilation* c, const char* path, const char* text);
bool gcs_replace_file(gcs_compilation* c, const char* temporary, const char* destination);
bool gcs_file_exists(const char* path);
bool gcs_remove_file(gcs_compilation* c, const char* path);
bool gcs_tool(gcs_compilation* c, const char* const* arguments, const char* capture);
char* gcs_build_path(gcs_compilation* c, const char* directory, const char* name);
bool gcs_project_layout(gcs_compilation* c, gcs_project* project);
bool gcs_import_project(gcs_compilation* c, gcs_project* project);
bool gcs_emit_contract(gcs_compilation* c, gcs_project* project, gcs_buffer* output);
bool gcs_emit_project(gcs_compilation* c, gcs_project* project);
bool gcs_build_project(gcs_compilation* c, gcs_project* project, u32 mode);
bool gcs_object_build(gcs_compilation* c, const char* source, const char* object, const char* include, bool force);
bool gcs_link_project(gcs_compilation* c, gcs_project* project, bool force);
bool gcs_emit_signature(gcs_compilation* c, gcs_method* method, gcs_buffer* output);
void gcs_emit_method(gcs_compilation* c, gcs_method* method);
void gcs_emit_entry(gcs_compilation* c);
bool gcs_emit_project_headers(gcs_compilation* c, gcs_project* project);
bool gcs_response_argument(gcs_compilation* c, gcs_buffer* output, const char* argument);
bool gcs_hash_file(gcs_compilation* c, const char* path, u64* hash);
bool gcs_stamp_matches(gcs_compilation* c, const char* path, u64 hash);
bool gcs_stamp_write(gcs_compilation* c, const char* path, u64 hash);
bool gcs_clean_project(gcs_compilation* c, gcs_project* project);
void gcs_record_artifact(gcs_compilation* c, const char* path);
bool gcs_merge_artifacts(gcs_compilation* c, const char* manifest);
bool gcs_is_declaration(gcs_parser* parser);
bool gcs_parse_member(gcs_parser* parser);

bool gcs_write_data(gcs_compilation* c, const char* path, const char* text);
char* gcs_read_data(gcs_compilation* c, const char* path);
