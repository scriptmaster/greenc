/*
 ** file_details.h - 2 functions.
 ** editor_file_details - Cache disk size and local modification time for an open document.
 ** editor_read_binary - Load unmodified bytes from the start of the same open stream.
 ** See also: file_details.c, document_io.c.
 */
#pragma once
#include "editor_document.h"
#include <stdio.h>

void editor_file_details(editor_document* document, const char* path);
bool editor_read_binary(editor_document* document, FILE* file);
