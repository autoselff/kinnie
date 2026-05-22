#ifndef LEXER_H
#define LEXER_H

#include "kinnie.h"

// Tokenizes a null-terminated source string into tokens[]; returns the number of tokens produced.
size_t tokenize(const char *src, Token tokens[]);

// Expands all `add "file"` directives by inlining the tokenized content of each file; returns the total token count written to output[].
size_t process_includes(Token tokens[], size_t token_count, Token output[], size_t max_tokens);

#endif
