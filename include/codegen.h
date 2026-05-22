#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "kinnie.h"

// Recursively converts a token sequence into C++ statements written to out at the given indentation level.
void convert_tokens_to_cpp(Token tokens[], size_t token_count, FILE *out, int indent, int is_main);

// Parses structs and functions from tokens, then writes a complete C++ source file to output_path; fills stats if non-NULL.
void convert_to_cpp(Token tokens[], size_t token_count, const char *output_path, CompileStats *stats);

#endif
