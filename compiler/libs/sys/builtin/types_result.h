//#file libs/sys/builtin/types_result.h
#pragma once
#include "types_intrinsic.h"

// -------------------------------------
// Result Types
// -------------------------------------

#define RESULT(T) struct { int error; T value; }

#define OK(T, v) ((RESULT(T)){.error = 0, .value = (v)})
#define ERR(T, e) ((RESULT(T)){.error = (e), .value = {0}})


// -------------------------------------
// Manual bootstrap result typedefs
// -------------------------------------

typedef RESULT(int) int_result;
typedef RESULT(buf32) buf32_result;
typedef RESULT(cstr) cstr_result;
typedef RESULT(string) string_result;
typedef RESULT(string_view) string_view_result;
typedef RESULT(string_list) string_list_result;


// -------------------------------------
// Generated-style ergonomic wrappers
// (manual until self-hosting/bootstrap)
// -------------------------------------

#define Ok(T) Ok_##T
#define Err(T) Err_##T


// -------------------------------------
// int
// -------------------------------------

#define Ok_int(v) OK(int, (v))
#define Err_int(e) ERR(int, (e))


// -------------------------------------
// buf32
// -------------------------------------

#define Ok_buf32(v) OK(buf32, (v))
#define Err_buf32(e) ERR(buf32, (e))


// -------------------------------------
// cstr
// -------------------------------------

#define Ok_cstr(v) OK(cstr, (v))
#define Err_cstr(e) ERR(cstr, (e))


// -------------------------------------
// string
// -------------------------------------

#define Ok_string(v) OK(string, (v))
#define Err_string(e) ERR(string, (e))


// -------------------------------------
// string_view
// -------------------------------------

#define Ok_string_view(v) OK(string_view, (v))
#define Err_string_view(e) ERR(string_view, (e))


// -------------------------------------
// string_list
// -------------------------------------

#define Ok_string_list(v) OK(string_list, (v))
#define Err_string_list(e) ERR(string_list, (e))
