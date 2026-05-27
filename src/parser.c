#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

Function functions[MAX_FUNCTIONS];
size_t   function_count = 0;
Struct   structs[MAX_STRUCTS];
size_t   struct_count = 0;

size_t find_block_end(Token tokens[], size_t token_count, size_t start) {
    int depth = 1;
    size_t j = start;
    while (j < token_count && tokens[j].type != TOK_EOF) {
        if (tokens[j].type == TOK_LBRACE) depth++;
        else if (tokens[j].type == TOK_RBRACE) {
            depth--;
            if (depth == 0) return j;
        }
        j++;
    }
    return j;
}

size_t copy_block(Token tokens[], size_t start, size_t end, Token dst[]) {
    size_t n = 0;
    for (size_t j = start; j < end && n < MAX_TOKENS - 1; j++)
        dst[n++] = tokens[j];
    dst[n].type = TOK_EOF;
    return n;
}

void parse_functions(Token tokens[], size_t token_count) {
    memset(functions, 0, sizeof(functions));
    function_count = 0;

    size_t i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (tokens[i].type == TOK_STRUCT) {
            i++;
            while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) i++;
            if (tokens[i].type == TOK_LBRACE) {
                size_t end = find_block_end(tokens, token_count, i + 1);
                i = end + 1;
            }
            continue;
        }
        if (tokens[i].type != TOK_FUN_START) { i++; continue; }
        i++;

        if (tokens[i].type != TOK_IDENT) {
            fprintf(stderr, "Expected function name after 'fun'\n");
            exit(1);
        }

        Function *f = &functions[function_count];
        snprintf(f->name, MAX_NAME_LEN, "%s", tokens[i].text);
        f->param_count = 0;
        i++;

        if (tokens[i].type == TOK_LBRACKET) {
            i++;
            while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                if (tokens[i].type != TOK_IDENT) {
                    fprintf(stderr, "Expected parameter name\n");
                    exit(1);
                }
                snprintf(f->param_names[f->param_count], MAX_NAME_LEN, "%s", tokens[i].text);
                i++;
                f->param_is_array[f->param_count] = 0;
                if (tokens[i].type == TOK_LSQUARE) {
                    f->param_is_array[f->param_count] = 1;
                    i++;
                    if (tokens[i].type != TOK_RSQUARE) {
                        fprintf(stderr, "Expected ']' after array parameter\n");
                        exit(1);
                    }
                    i++;
                }
                f->param_count++;
                if (tokens[i].type == TOK_COMMA) i++;
            }
            if (tokens[i].type != TOK_RBRACKET) {
                fprintf(stderr, "Expected ')' after parameters\n");
                exit(1);
            }
            i++;
        }

        if (tokens[i].type != TOK_LBRACE) {
            fprintf(stderr, "Expected '{' after function signature\n");
            exit(1);
        }
        i++;

        size_t end = find_block_end(tokens, token_count, i);
        f->token_count = copy_block(tokens, i, end, f->tokens);
        function_count++;
        i = end + 1;
    }
}

Function *get_function(const char *name) {
    for (size_t i = 0; i < function_count; i++)
        if (strcmp(functions[i].name, name) == 0) return &functions[i];
    return NULL;
}

void parse_structs(Token tokens[], size_t token_count) {
    memset(structs, 0, sizeof(structs));
    struct_count = 0;

    size_t i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (tokens[i].type != TOK_STRUCT) { i++; continue; }
        i++;
        if (tokens[i].type != TOK_IDENT) continue;
        if (tokens[i + 1].type != TOK_LBRACE) { i++; continue; }
        if (struct_count >= MAX_STRUCTS) break;

        Struct *c = &structs[struct_count];
        snprintf(c->name, MAX_NAME_LEN, "%s", tokens[i].text);
        c->field_count = 0;
        c->method_count = 0;
        i += 2;

        size_t end = find_block_end(tokens, token_count, i);
        size_t j = i;
        while (j < end) {
            if (tokens[j].type == TOK_IDENT && j + 1 < end && tokens[j + 1].type == TOK_IDENT && tokens[j + 2].type != TOK_ASSIGN && tokens[j + 2].type != TOK_LSQUARE) {
                int is_struct_type = 0;
                for (size_t si = 0; si < struct_count; si++) {
                    if (strcmp(tokens[j].text, structs[si].name) == 0) {
                        is_struct_type = 1;
                        break;
                    }
                }
                if (is_struct_type && c->field_count < MAX_STRUCT_FIELDS) {
                    snprintf(c->field_names[c->field_count], MAX_NAME_LEN, "%s", tokens[j + 1].text);
                    snprintf(c->field_struct_types[c->field_count], MAX_NAME_LEN, "%s", tokens[j].text);
                    c->field_is_struct[c->field_count] = 1;
                    c->field_is_string[c->field_count] = 0;
                    c->field_is_array[c->field_count] = 0;
                    snprintf(c->field_defaults[c->field_count], MAX_STRING_LEN, "");
                    c->field_count++;
                    j += 2;
                    continue;
                }
            }
            if (tokens[j].type == TOK_VAR && j + 1 < end && tokens[j + 1].type == TOK_IDENT) {
                j++;
                if (c->field_count >= MAX_STRUCT_FIELDS) { j++; continue; }
                snprintf(c->field_names[c->field_count], MAX_NAME_LEN, "%s", tokens[j].text);
                j++;
                if (tokens[j].type == TOK_ASSIGN) j++;
                if (tokens[j].type == TOK_LSQUARE) {
                    c->field_is_array[c->field_count] = 1;
                    c->field_is_string[c->field_count] = 0;
                    c->field_is_struct[c->field_count] = 0;
                    char arr_init[MAX_STRING_LEN] = "_KnTable{";
                    int arr_len = strlen(arr_init);
                    int first = 1;
                    j++;
                    while (j < end && tokens[j].type != TOK_RSQUARE) {
                        if (tokens[j].type == TOK_NUMBER) {
                            if (!first) arr_len += snprintf(arr_init + arr_len, MAX_STRING_LEN - arr_len, ", ");
                            arr_len += snprintf(arr_init + arr_len, MAX_STRING_LEN - arr_len, "_KnVal(%s)", tokens[j].text);
                            first = 0;
                        } else if (tokens[j].type == TOK_COMMA) {
                            // skip commas
                        }
                        j++;
                    }
                    snprintf(arr_init + arr_len, MAX_STRING_LEN - arr_len, "}");
                    snprintf(c->field_defaults[c->field_count], MAX_STRING_LEN, "%s", arr_init);
                    if (j < end && tokens[j].type == TOK_RSQUARE) j++;
                } else if (tokens[j].type == TOK_STRING) {
                    c->field_is_string[c->field_count] = 1;
                    c->field_is_array[c->field_count] = 0;
                    c->field_is_struct[c->field_count] = 0;
                    snprintf(c->field_defaults[c->field_count], MAX_STRING_LEN, "\"%s\"", tokens[j].text);
                    j++;
                } else if (tokens[j].type == TOK_NUMBER) {
                    c->field_is_string[c->field_count] = 0;
                    c->field_is_array[c->field_count] = 0;
                    c->field_is_struct[c->field_count] = 0;
                    snprintf(c->field_defaults[c->field_count], MAX_STRING_LEN, "%s", tokens[j].text);
                    j++;
                } else if (tokens[j].type == TOK_MINUS && tokens[j + 1].type == TOK_NUMBER) {
                    c->field_is_string[c->field_count] = 0;
                    c->field_is_array[c->field_count] = 0;
                    c->field_is_struct[c->field_count] = 0;
                    snprintf(c->field_defaults[c->field_count], MAX_STRING_LEN, "-%s", tokens[j + 1].text);
                    j += 2;
                } else {
                    c->field_is_string[c->field_count] = 0;
                    c->field_is_array[c->field_count] = 0;
                    c->field_is_struct[c->field_count] = 0;
                    snprintf(c->field_defaults[c->field_count], MAX_STRING_LEN, "0");
                }
                c->field_count++;
                continue;
            }
            if (tokens[j].type == TOK_FUN_START && j + 1 < end && tokens[j + 1].type == TOK_IDENT) {
                j++;
                if (c->method_count >= MAX_STRUCT_METHODS) { j++; continue; }
                Function *f = &c->methods[c->method_count];
                strncpy(f->name, tokens[j].text, MAX_NAME_LEN - 1);
                f->param_count = 0;
                j++;
                if (tokens[j].type == TOK_LBRACKET) {
                    j++;
                    while (tokens[j].type != TOK_RBRACKET && tokens[j].type != TOK_EOF) {
                        if (tokens[j].type == TOK_IDENT && f->param_count < MAX_FUNC_PARAMS) {
                            strncpy(f->param_names[f->param_count], tokens[j].text, MAX_NAME_LEN - 1);
                            f->param_is_array[f->param_count] = 0;
                            f->param_count++;
                        }
                        j++;
                        if (tokens[j].type == TOK_LSQUARE) {
                            if (f->param_count > 0) {
                                f->param_is_array[f->param_count - 1] = 1;
                                j++;
                                if (tokens[j].type == TOK_RSQUARE) j++;
                            }
                        }
                        if (tokens[j].type == TOK_COMMA) j++;
                    }
                    if (tokens[j].type == TOK_RBRACKET) j++;
                }
                if (tokens[j].type != TOK_LBRACE) continue;
                j++;
                size_t mend = find_block_end(tokens, token_count, j);
                f->token_count = copy_block(tokens, j, mend, f->tokens);
                c->method_count++;
                j = mend + 1;
                continue;
            }
            j++;
        }
        struct_count++;
        i = end + 1;
    }
}
