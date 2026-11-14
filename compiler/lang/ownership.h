#pragma once

/* The symbol stack is lexical. Branches copy availability bits only. Borrow
 * dependencies use indices so growing the stack never invalidates a loan. */
typedef enum { OWN_LOCAL, READ_LOCAL, STACK_LOCAL } ownership_kind;
typedef struct {
    const char* name;
    size_t length, origin;
    ownership_kind kind;
    unsigned state;
    const char* field;
    size_t field_length;
    bool must_drain;
} ownership_binding;

enum { OWNER_LIVE = 1, OWNER_SPENT = 2 };
typedef struct ownership_flow {
    ownership_binding* bindings;
    size_t count, capacity;
    bool reachable;
    const char* contracts;
    int return_contract;
} ownership_flow;

static bool hosted_statement(hosted_parser* p);

static bool ownership_error(const char* reason, const ownership_binding* symbol) {
    print(reason);
    if (symbol) {
        sys_write(symbol->name, symbol->length);
        sys_write("\n", 1);
    }

    return false;
}

static size_t ownership_find(ownership_flow* flow, hosted_parser token) {
    for (size_t i = flow->count; i > 0; --i) {
        ownership_binding* binding = &flow->bindings[i - 1];
        if (binding->field || binding->length != token.length) { continue; }
        size_t j = 0;
        while (j < token.length && binding->name[j] == token.token[j]) { ++j; }
        if (j == token.length) { return i - 1; }
    }

    return (size_t)-1;
}

static bool ownership_track(ownership_flow* flow, ownership_binding binding) {
    if (flow->count == flow->capacity) {
        size_t capacity = flow->capacity ? flow->capacity * 2 : 8;
        if (capacity <= flow->capacity || capacity > (size_t)-1 / sizeof(binding)) { return false; }

        ownership_binding* next = greenc_malloc(capacity * sizeof(binding));
        if (!next) { return false; }
        if (flow->count) { greenc_memcpy(next, flow->bindings, flow->count * sizeof(binding)); }
        greenc_free(flow->bindings);
        flow->bindings = next;
        flow->capacity = capacity;
    }

    flow->bindings[flow->count++] = binding;
    return true;
}

static bool ownership_use(ownership_flow* flow, size_t index) {
    while (index < flow->count) {
        ownership_binding* b = &flow->bindings[index];
        if (b->state & OWNER_SPENT) { return ownership_error("ownership: use after move or release", b); }
        index = b->origin;
    }

    return true;
}

static bool ownership_consume(ownership_flow* flow, size_t index) {
    ownership_binding* b = &flow->bindings[index];
    if (!ownership_use(flow, index)) { return false; }
    if (b->kind != OWN_LOCAL) { return ownership_error("ownership: cannot consume borrowed or stack storage", b); }
    b->state = OWNER_SPENT;
    return true;
}

static bool ownership_leave(hosted_parser* p, size_t scope) {
    ownership_flow* flow = p->ownership;
    for (size_t i = scope; flow->reachable && i < flow->count; ++i) {
        ownership_binding* b = &flow->bindings[i];
        if ((!b->field || b->must_drain) && b->kind == OWN_LOCAL && (b->state & OWNER_LIVE)) {
            return ownership_error("ownership: owner leaves scope without transfer or release", b);
        }
    }

    flow->count = scope;
    return true;
}

static bool ownership_release_call(hosted_parser t) {
    return hosted_word(&t, "free") || hosted_word(&t, "greenc_free") ||
           hosted_word(&t, "release") || hosted_word(&t, "greenc_release") ||
           hosted_word(&t, "fclose") || hosted_word(&t, "ui_window_destroy") ||
           hosted_word(&t, "ui_renderer_destroy");
}

static hosted_parser ownership_tokens(const char* start) {
    hosted_parser t = {.cursor = (char*)start};
    hosted_next(&t);

    return t;
}

#include "call_contracts.h"

static bool ownership_expression(hosted_parser* p, const char* start, const char* end, bool transfer) {
    ownership_flow* flow = p->ownership;
    hosted_parser t = ownership_tokens(start);
    bool member = false;
    while (t.length && t.token < end) {
        hosted_parser last = t;
        size_t index = member ? (size_t)-1 : contract_place(flow, &last);
        hosted_parser signature;
        if (!member && contract_lookup(flow, t, &signature)) {
            hosted_parser args = t;
            hosted_next(&args);
            if (hosted_word(&args, "(")) {
                hosted_parser finish = args;
                hosted_parser first = args;
                hosted_next(&first);
                if (!hosted_group(&finish) ||
                    !ownership_expression(p, first.token, finish.token, false) ||
                    !contract_parameters(p, t, args, false)) { return false; }
                t = finish;
                member = false;
                continue;
            }
        }
        if (index < flow->count && !ownership_use(flow, index)) { return false; }
        if (!member && ownership_release_call(t)) {
            hosted_parser arg = t;
            hosted_next(&arg);
            if (hosted_take(&arg, "(")) {
                size_t target = contract_place(flow, &arg);
                hosted_next(&arg);
                if (target < flow->count && hosted_word(&arg, ")")) {
                    if (!ownership_consume(flow, target)) { return false; }
                    t = arg;
                }
            }
        } else if (transfer && index < flow->count && t.token == start) {
            if (!ownership_consume(flow, index)) { return false; }
        }

        if (last.token != t.token && index < flow->count) { t = last; }
        member = hosted_word(&t, ".") || (hosted_word(&t, ">") && t.token > start && t.token[-1] == '-');
        hosted_next(&t);
    }

    return true;
}

/* Recognize only an exact trailing marker on the declaration's own line. */
static int ownership_marker(const char* after) {
    while (*after == ' ' || *after == '\t') { ++after; }
    if (after[0] != '/' || after[1] != '/' || after[2] != '/') { return -1; }
    after += 3;
    size_t n = 0;
    while (hosted_identifier(after[n])) { ++n; }
    if (greenc_str_equals(after, n, "move")) { return OWN_LOCAL; }
    if (greenc_str_equals(after, n, "read")) { return READ_LOCAL; }
    if (greenc_str_equals(after, n, "stack")) { return STACK_LOCAL; }

    return -1;
}

static bool ownership_statement(hosted_parser* p, const char* start, const char* end) {
    ownership_flow* flow = p->ownership;
    if (!flow->reachable) { return true; }
    hosted_parser t = ownership_tokens(start), previous = t, name = {0}, rhs = {0};
    bool returning = hosted_word(&t, "return");
    while (t.length && t.token < end && !hosted_word(&t, ";")) {
        if (!rhs.length && hosted_word(&t, "=")) {
            name = previous;
            rhs = t;
            hosted_next(&rhs);
        }
        previous = t;
        hosted_next(&t);
    }

    if (!hosted_word(&t, ";")) { return false; }
    int marker = ownership_marker(t.cursor);
    if (marker >= 0) {
        if (!rhs.length || !name.length || !hosted_identifier(*name.token) || name.token == start) {
            return ownership_error("ownership: marker requires an initialized local declaration", NULL);
        }

        size_t origin = (size_t)-1;
        hosted_parser init = rhs;
        if (hosted_word(&init, "&")) { hosted_next(&init); }
        hosted_parser last = init;
        origin = contract_place(flow, &last);
        if (marker == READ_LOCAL && origin == (size_t)-1) {
            hosted_parser argument = rhs;
            while (argument.length && argument.token < t.token) {
                size_t candidate = ownership_find(flow, argument);
                if (candidate < flow->count) {
                    if (origin < flow->count && origin != candidate) {
                        return ownership_error("ownership: ambiguous borrow origin", NULL);
                    }
                    origin = candidate;
                }
                hosted_next(&argument);
            }
        }

        hosted_parser after = last;
        hosted_next(&after);
        bool direct = hosted_word(&after, ";") && init.token == rhs.token;
        int result_kind = contract_result(flow, rhs);
        if (result_kind >= 0 && result_kind != marker) {
            return ownership_error("ownership: incompatible return contract", NULL);
        }
        if (!ownership_expression(p, rhs.token, t.token, marker == OWN_LOCAL && direct)) { return false; }

        return ownership_track(flow, (ownership_binding){name.token, name.length,
            marker == READ_LOCAL ? origin : (size_t)-1, (ownership_kind)marker, OWNER_LIVE});
    }

    if (!returning && (contract_result(flow, ownership_tokens(start)) == OWN_LOCAL ||
        (rhs.length && contract_result(flow, rhs) == OWN_LOCAL))) {
        return ownership_error("ownership: owned result requires an explicit move", NULL);
    }

    if (!ownership_expression(p, start, t.token, false)) { return false; }
    if (rhs.length && name.length && name.token > start) {
        hosted_parser left = ownership_tokens(start);
        hosted_next(&left);
        bool field = hosted_word(&left, ".") || hosted_word(&left, "-");
        size_t source = ownership_find(flow, rhs);
        hosted_parser tail = rhs;
        hosted_next(&tail);
        if (!field && source < flow->count && hosted_word(&tail, ";") &&
            flow->bindings[source].kind != STACK_LOCAL) {
            return ownership_error("ownership: pointer copy requires a move or read declaration", &flow->bindings[source]);
        }

        if (field && source < flow->count && flow->bindings[source].kind == OWN_LOCAL &&
            hosted_word(&tail, ";") && !ownership_consume(flow, source)) { return false; }
    }

    if (returning) {
        hosted_parser result = ownership_tokens(start);
        hosted_next(&result);
        bool address = hosted_take(&result, "&");
        size_t target = contract_place(flow, &result);
        if (address && target < flow->count) {
            return ownership_error("ownership: local address escapes", &flow->bindings[target]);
        }
        int returned_call = contract_result(flow, result);
        hosted_next(&result);
        if (flow->return_contract >= 0) {
            int actual = target < flow->count && hosted_word(&result, ";") ?
                (int)flow->bindings[target].kind : returned_call;
            if (actual != flow->return_contract) {
                return ownership_error("ownership: return does not satisfy function contract", NULL);
            }
        }

        if (target < flow->count && hosted_word(&result, ";")) {
            ownership_binding* b = &flow->bindings[target];
            if (b->kind == READ_LOCAL && b->origin < flow->count) {
                return ownership_error("ownership: local borrow escapes", b);
            }
            if (b->kind == OWN_LOCAL && !ownership_consume(flow, target)) { return false; }
        }

        size_t count = flow->count;
        if (!ownership_leave(p, 0)) { return false; }
        flow->count = count;
        flow->reachable = false;
    }

    return true;
}

static bool ownership_branch(hosted_parser* p, const char* condition, bool conditional, bool loop) {
    ownership_flow* flow = p->ownership;
    size_t count = flow->count;
    unsigned char* states = greenc_malloc(count * 2 + 1);
    if (!states) { return false; }
    bool entry = flow->reachable;
    for (size_t i = 0; i < count; ++i) { states[i] = (unsigned char)flow->bindings[i].state; }

    /* A failed allocation has no resource on the null branch. */
    hosted_parser guard = ownership_tokens(condition);
    hosted_next(&guard);
    size_t null_else = (size_t)-1;
    bool negated = hosted_take(&guard, "!");
    if (conditional) {
        size_t owner = ownership_find(flow, guard);
        hosted_next(&guard);
        if (owner < count && hosted_word(&guard, ")") && flow->bindings[owner].kind == OWN_LOCAL) {
            if (negated) { flow->bindings[owner].state = OWNER_SPENT; }
            else { null_else = owner; }
        }
    }

    bool ok = hosted_statement(p);
    bool then_live = flow->reachable;
    for (size_t i = 0; i < count; ++i) {
        states[count + i] = (unsigned char)flow->bindings[i].state;
        flow->bindings[i].state = states[i];
    }
    flow->reachable = entry;
    if (null_else < count) { flow->bindings[null_else].state = OWNER_SPENT; }
    if (ok && conditional && hosted_take(p, "else")) { ok = hosted_statement(p); }
    bool else_live = flow->reachable;
    for (size_t i = 0; ok && i < count; ++i) {
        if (loop && then_live && states[count + i] != states[i]) {
            ok = ownership_error("ownership: outer owner changes across a loop iteration", &flow->bindings[i]);
        }
        flow->bindings[i].state = (then_live ? states[count + i] : 0) |
                                  (else_live ? flow->bindings[i].state : 0);
    }

    flow->reachable = then_live || else_live;
    greenc_free(states);
    return ok;
}
