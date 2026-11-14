/*
 ** directory_listing.c - 3 functions.
 ** collect_entry - Append a discovered filesystem item to a listing.
 ** compare_entries - Sort directories first and compare names case-insensitively.
 ** insert_children - Insert a sorted directory listing into the visible tree.
 ** See also: tree_internal.h.
 */
#include "tree_internal.h"

static bool collect_entry(void* context, const char* name, bool folder, bool hidden) {
    directory_listing* listing = context;
    if (listing->count == listing->capacity) {
        size_t capacity = listing->capacity ? listing->capacity * 2 : 32;
        editor_tree_row* rows = realloc(listing->rows, capacity * sizeof(*rows)); ///move
        if (!rows) { return false; }

        listing->rows = rows;
        listing->capacity = capacity;
    }

    editor_tree_row row = {.depth = listing->depth, .folder = folder, .hidden = hidden || name[0] == '.'};
    size_t length = strlen(listing->parent);
    const char* slash = length && listing->parent[length - 1] == '\\' ? "" : "\\";
    if (snprintf(row.path, sizeof(row.path), "%s%s%s", listing->parent, slash, name) >= (int)sizeof(row.path)) {
        return false;
    }

    row.name_offset = length + strlen(slash);
    listing->rows[listing->count++] = row;

    return true;
}

static int compare_entries(const void* left, const void* right) {
    const editor_tree_row* a = left;
    const editor_tree_row* b = right;
    if (a->folder != b->folder) { return a->folder ? -1 : 1; }

    return _stricmp(a->path + a->name_offset, b->path + b->name_offset);
}

bool insert_children(editor_tree* tree, size_t index, const char* path, size_t depth) {
    directory_listing listing = {.parent = path, .depth = depth};
    bool success = sys_directory_each(path, collect_entry, &listing);
    if (success && listing.count) {
        qsort(listing.rows, listing.count, sizeof(*listing.rows), compare_entries);
        editor_tree_row* rows = realloc(tree->rows, (tree->count + listing.count) * sizeof(*rows)); ///move
        if (rows) {
            memmove(rows + index + listing.count, rows + index, (tree->count - index) * sizeof(*rows));
            memcpy(rows + index, listing.rows, listing.count * sizeof(*rows));
            tree->rows = rows;
            tree->count += listing.count;
        } else {
            success = false;
        }
    }

    free(listing.rows);

    return success;
}
