#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

static const struct { const char *kw; TokenType type; } KEYWORDS[] = {
    {"var",        TOK_VAR},
    {"ret",        TOK_RETURN},
    {"out",        TOK_PRINT},
    {"rep",        TOK_LOOP_START},
    {"fun",        TOK_FUN_START},
    {"if",         TOK_IF_START},
    {"else",       TOK_ELSE},
    {"end",        TOK_END},
    {"keyPressed", TOK_KEY_PRESSED},
    {"keyDown",    TOK_KEY_DOWN},
    {"add",        TOK_ADD},
    {"stop",       TOK_STOP},
    {"str",        TOK_STRUCT},
    {"and",        TOK_AND},
    {"or",         TOK_OR},
    {"not",        TOK_NOT},
    {"qbit",       TOK_QBIT},
};

static TokenType lookup_keyword(const char *text) {
    for (size_t k = 0; k < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); k++)
        if (strcmp(text, KEYWORDS[k].kw) == 0) return KEYWORDS[k].type;
    return TOK_IDENT;
}

static void copy_lexeme(Token *t, const char *src, size_t start, size_t end) {
    size_t len = end - start;
    if (len >= MAX_STRING_LEN) len = MAX_STRING_LEN - 1;
    memcpy(t->text, src + start, len);
    t->text[len] = '\0';
}

size_t tokenize(const char *src, Token tokens[]) {
    size_t i = 0, t = 0;
    while (src[i]) {
        if (isspace((unsigned char)src[i])) { i++; continue; }

        if (src[i] == '/' && src[i + 1] == '/') {
            while (src[i] && src[i] != '\n') i++;
            continue;
        }

        if (t + 1 >= MAX_TOKENS) {
            fprintf(stderr, "Error: too many tokens (max %d). Split your file or increase MAX_TOKENS.\n", MAX_TOKENS);
            exit(1);
        }

        if (src[i] == '"') {
            i++;
            size_t start = i;
            while (src[i] && src[i] != '"') i++;
            copy_lexeme(&tokens[t], src, start, i);
            tokens[t].type = TOK_STRING;
            if (src[i] == '"') i++;
            t++;
            continue;
        }

        if (isdigit((unsigned char)src[i]) || (src[i] == '.' && isdigit((unsigned char)src[i + 1]))) {
            size_t start = i;
            while (isdigit((unsigned char)src[i])) i++;
            if (src[i] == '.' && isdigit((unsigned char)src[i + 1])) {
                i++;
                while (isdigit((unsigned char)src[i])) i++;
            }
            copy_lexeme(&tokens[t], src, start, i);
            tokens[t].type = TOK_NUMBER;
            t++;
            continue;
        }

        if (isalpha((unsigned char)src[i])) {
            size_t start = i;
            while (isalnum((unsigned char)src[i])) i++;
            copy_lexeme(&tokens[t], src, start, i);
            tokens[t].type = lookup_keyword(tokens[t].text);
            t++;
            continue;
        }

        if (src[i] == '+' && src[i + 1] == '+') { tokens[t++].type = TOK_INCREMENT; i += 2; continue; }
        if (src[i] == '-' && src[i + 1] == '-') { tokens[t++].type = TOK_DECREMENT; i += 2; continue; }

        if (src[i + 1] == '=') {
            TokenType tt = TOK_UNKNOWN;
            switch (src[i]) {
                case '=': tt = TOK_EQUALS;       break;
                case '!': tt = TOK_NOT_EQUALS;   break;
                case '>': tt = TOK_MORE_EQUALS;  break;
                case '<': tt = TOK_LESS_EQUALS;  break;
                case '+': tt = TOK_PLUS_ASSIGN;  break;
                case '-': tt = TOK_MINUS_ASSIGN; break;
                case '*': tt = TOK_MUL_ASSIGN;   break;
                case '/': tt = TOK_DIV_ASSIGN;   break;
                default: break;
            }
            if (tt != TOK_UNKNOWN) {
                tokens[t++].type = tt;
                i += 2;
                continue;
            }
        }

        TokenType tt;
        switch (src[i]) {
            case '{': tt = TOK_LBRACE;   break;
            case '}': tt = TOK_RBRACE;   break;
            case '(': tt = TOK_LBRACKET; break;
            case ')': tt = TOK_RBRACKET; break;
            case '[': tt = TOK_LSQUARE;  break;
            case ']': tt = TOK_RSQUARE;  break;
            case ',': tt = TOK_COMMA;    break;
            case '=': tt = TOK_ASSIGN;   break;
            case '>': tt = TOK_MORE;     break;
            case '<': tt = TOK_LESS;     break;
            case '+': tt = TOK_PLUS;     break;
            case '-': tt = TOK_MINUS;    break;
            case '*': tt = TOK_MUL;      break;
            case '/': tt = TOK_DIV;      break;
            case '%': tt = TOK_MOD;      break;
            case '.': tt = TOK_DOT;      break;
            default:  tt = TOK_UNKNOWN;  break;
        }
        tokens[t++].type = tt;
        i++;
    }
    tokens[t].type = TOK_EOF;
    return t;
}

size_t process_includes(Token tokens[], size_t token_count, Token output[], size_t max_tokens) {
    size_t out_idx = 0, i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (tokens[i].type != TOK_ADD) {
            if (out_idx < max_tokens) output[out_idx++] = tokens[i];
            i++;
            continue;
        }

        if (tokens[i + 1].type != TOK_STRING) {
            if (out_idx < max_tokens) output[out_idx++] = tokens[i];
            i++;
            continue;
        }

        i++;
        const char *filename = tokens[i].text;
        i++;

        FILE *f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "Cannot open file: %s\n", filename);
            exit(1);
        }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        if (size > 0) {
            char *source = malloc(size + 1);
            fread(source, 1, size, f);
            source[size] = '\0';
            fclose(f);

            Token temp[MAX_TOKENS];
            size_t tn = tokenize(source, temp);
            for (size_t j = 0; j < tn && out_idx < max_tokens; j++)
                if (temp[j].type != TOK_EOF) output[out_idx++] = temp[j];
            free(source);
        } else {
            fclose(f);
        }
    }
    if (out_idx < max_tokens) output[out_idx++].type = TOK_EOF;
    return out_idx;
}
