#ifndef PARSER_H
#define PARSER_H

#include "kinnie.h"

extern Function functions[MAX_FUNCTIONS];
extern size_t   function_count;
extern Struct   structs[MAX_STRUCTS];
extern size_t   struct_count;

// Returns the index of the closing `}` that matches the `{` depth starting at position start.
size_t find_block_end(Token tokens[], size_t token_count, size_t start);

// Copies tokens[start..end) into dst[] and appends a TOK_EOF sentinel; returns the number of tokens copied.
size_t copy_block(Token tokens[], size_t start, size_t end, Token dst[]);

// Scans tokens and populates the global functions[] array; struct bodies are skipped.
void parse_functions(Token tokens[], size_t token_count);

// Scans tokens and populates the global structs[] array with field and method definitions.
void parse_structs(Token tokens[], size_t token_count);

// Returns a pointer to the Function with the given name, or NULL if not found.
Function *get_function(const char *name);

#endif
