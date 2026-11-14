#pragma once

/* One contract per declaration, shared between independently emitted units.
 * The argument list is positional. fields(T) binds T's annotated members to
 * that parameter; no type guesses based on member names are necessary. */
static bool contract_same(hosted_parser a, hosted_parser b) {
    if (a.length != b.length) { return false; }
    for (size_t i = 0; i < a.length; ++i) {
        if (a.token[i] != b.token[i]) { return false; }
    }

    return true;
}

static int contract_kind(hosted_parser t) {
    if (hosted_word(&t, "move")) { return OWN_LOCAL; }
    if (hosted_word(&t, "read")) { return READ_LOCAL; }
    if (hosted_word(&t, "stack")) { return STACK_LOCAL; }
    return -1;
}

static bool contract_validate(const char* source) {
    if (!source) { return true; }
    hosted_parser t = ownership_tokens(source);
    while (t.length) {
        if (hosted_take(&t, "field")) {
            if (!t.length) { return false; }
            hosted_next(&t);
            if (!hosted_take(&t, ".")) { return false; }
            hosted_next(&t);
            if (contract_kind(t) < 0) { return false; }
            hosted_next(&t);
        } else if (hosted_take(&t, "fn")) {
            hosted_next(&t);
            if (!hosted_take(&t, "(")) { return false; }
            while (!hosted_word(&t, ")")) {
                if (hosted_take(&t, "fields") || hosted_take(&t, "drain")) {
                    if (!hosted_take(&t, "(")) { return false; }
                    hosted_next(&t);
                    if (!hosted_take(&t, ")")) { return false; }
                } else {
                    if (contract_kind(t) < 0 && !hosted_word(&t, "plain")) { return false; }
                    hosted_next(&t);
                }
                if (!hosted_take(&t, ",")) { break; }
            }
            if (!hosted_take(&t, ")")) { return false; }
            if (contract_kind(t) < 0 && !hosted_word(&t, "plain")) { return false; }
            hosted_next(&t);
        } else { return false; }
        if (!hosted_take(&t, ";")) { return false; }
    }

    return !t.failed;
}

static bool contract_lookup(ownership_flow* flow, hosted_parser name, hosted_parser* args) {
    if (!flow->contracts || !name.length || !hosted_identifier(*name.token)) { return false; }
    hosted_parser t = ownership_tokens(flow->contracts);
    while (t.length) {
        if (hosted_take(&t, "fn") && contract_same(name, t)) {
            hosted_next(&t);
            hosted_next(&t);
            *args = t;
            return true;
        }
        while (t.length && !hosted_take(&t, ";")) { hosted_next(&t); }
    }

    return false;
}

static int contract_result(ownership_flow* flow, hosted_parser name) {
    hosted_parser args;
    if (!contract_lookup(flow, name, &args)) { return -1; }
    unsigned nested = 0;
    while (args.length) {
        if (hosted_word(&args, ")") && !nested) { break; }
        if (hosted_word(&args, "(")) { ++nested; }
        else if (hosted_word(&args, ")")) { --nested; }
        hosted_next(&args);
    }
    hosted_next(&args);
    return contract_kind(args);
}

static size_t contract_place(ownership_flow* flow, hosted_parser* t) {
    hosted_parser base = *t, member = base;
    hosted_next(&member);
    bool field = hosted_take(&member, ".") ||
                 (hosted_take(&member, "-") && hosted_take(&member, ">"));
    if (field) {
        for (size_t i = flow->count; i > 0; --i) {
            ownership_binding* b = &flow->bindings[i - 1];
            if (!b->field) { continue; }
            hosted_parser root = {.token = b->name, .length = b->length};
            hosted_parser key = {.token = b->field, .length = b->field_length};
            if (contract_same(root, base) && contract_same(key, member)) {
                *t = member;
                return i - 1;
            }
        }
    }

    return ownership_find(flow, base);
}

static bool contract_fields(ownership_flow* flow, hosted_parser type, hosted_parser root, bool drain) {
    hosted_parser t = ownership_tokens(flow->contracts);
    while (t.length) {
        if (hosted_take(&t, "field") && contract_same(t, type)) {
            hosted_next(&t);
            hosted_next(&t);
            hosted_parser field = t;
            hosted_next(&t);
            if (!ownership_track(flow, (ownership_binding){root.token, root.length, (size_t)-1,
                    contract_kind(t), OWNER_LIVE, field.token, field.length, drain})) { return false; }
        }
        while (t.length && !hosted_take(&t, ";")) { hosted_next(&t); }
    }

    return true;
}

/* Leave the cursor on the comma or closing parenthesis. */
static bool contract_argument_end(hosted_parser* arg, hosted_parser* last) {
    while (arg->length && !hosted_word(arg, ",") && !hosted_word(arg, ")")) {
        *last = *arg;
        if (hosted_word(arg, "(") || hosted_word(arg, "[") || hosted_word(arg, "{")) {
            if (!hosted_group(arg)) { return false; }
        } else { hosted_next(arg); }
    }

    return arg->length != 0;
}

static bool contract_parameters(hosted_parser* p, hosted_parser callee, hosted_parser args, bool definition) {
    hosted_parser spec;
    if (!contract_lookup(p->ownership, callee, &spec)) { return true; }
    if (!hosted_take(&args, "(")) { return true; }
    while (!hosted_word(&spec, ")")) {
        hosted_parser first = args, last = args, type = {0};
        if (!contract_argument_end(&args, &last)) { return false; }
        int kind = contract_kind(spec);
        bool drain = hosted_word(&spec, "drain");
        if (hosted_take(&spec, "fields") || hosted_take(&spec, "drain")) {
            hosted_next(&spec);
            type = spec;
            hosted_next(&spec);
            hosted_next(&spec);
        } else { hosted_next(&spec); }

        if (definition && type.length) {
            if (!contract_fields(p->ownership, type, last, drain)) { return false; }
        } else if (!definition && drain) {
            if (hosted_word(&first, "&")) { hosted_next(&first); }
            hosted_parser root = first;
            hosted_next(&first);
            if (first.token != args.token) {
                return ownership_error("ownership: drain needs a direct aggregate argument", NULL);
            }
            bool known = false;
            for (size_t i = 0; i < p->ownership->count; ++i) {
                ownership_binding* field = &p->ownership->bindings[i];
                hosted_parser object = {.token = field->name, .length = field->length};
                if (field->field && field->kind == OWN_LOCAL && contract_same(root, object)) {
                    known = true;
                    if (!ownership_consume(p->ownership, i)) { return false; }
                }
            }
            if (!known) { return ownership_error("ownership: aggregate fields are untracked", NULL); }
        } else if (definition && kind >= 0) {
            if (!ownership_track(p->ownership, (ownership_binding){last.token, last.length,
                    (size_t)-1, kind, OWNER_LIVE, NULL, 0})) { return false; }
        } else if (!definition && kind == OWN_LOCAL) {
            size_t id = contract_place(p->ownership, &first);
            hosted_next(&first);
            if (id >= p->ownership->count || first.token != args.token) {
                return ownership_error("ownership: move argument requires a tracked owner", NULL);
            }
            if (!ownership_consume(p->ownership, id)) { return false; }
        }

        if (!hosted_take(&spec, ",")) { break; }
        if (!hosted_take(&args, ",")) { return ownership_error("ownership: parameter count mismatch", NULL); }
    }

    if (!hosted_word(&args, ")") && !hosted_word(&args, "void")) {
        return ownership_error("ownership: parameter count mismatch", NULL);
    }
    return true;
}

static bool contract_enter(hosted_parser* p, hosted_parser callee, hosted_parser args) {
    p->ownership->return_contract = contract_result(p->ownership, callee);
    return contract_parameters(p, callee, args, true);
}
