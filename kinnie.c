#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __has_include
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#    define HAVE_SDL 1
#  else
#    define HAVE_SDL 0
#  endif
#else
#  define HAVE_SDL 0
#endif

#define KINNIE_VERSION "2.2.0"
#define MAX_TOKENS 2048
#define MAX_EXPANDED_TOKENS 8192
#define MAX_FUNCTIONS 64
#define MAX_NAME_LEN 32
#define MAX_STRING_LEN 128
#define MAX_FUNC_PARAMS 16
#define ARG_BUF_LEN 256

typedef enum {
    TOK_VAR, TOK_PRINT, TOK_PRINTL, TOK_IDENT, TOK_NUMBER, TOK_STRING,
    TOK_ASSIGN,
    TOK_MORE, TOK_LESS, TOK_EQUALS, TOK_NOT_EQUALS,
    TOK_MORE_EQUALS, TOK_LESS_EQUALS,
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV, TOK_MOD,
    TOK_EOF,
    TOK_LOOP_START, TOK_FUN_START, TOK_IF_START, TOK_ELSE, TOK_END,
    TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET, TOK_LSQUARE, TOK_RSQUARE,
    TOK_COMMA, TOK_DOT,
    TOK_RETURN, TOK_KEY_PRESSED, TOK_KEY_DOWN, TOK_ADD, TOK_STOP,
    TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char text[MAX_STRING_LEN];
} Token;

typedef struct {
    char name[MAX_NAME_LEN];
    Token tokens[MAX_TOKENS];
    size_t token_count;
    char param_names[MAX_FUNC_PARAMS][MAX_NAME_LEN];
    size_t param_count;
} Function;

static Function functions[MAX_FUNCTIONS];
static size_t function_count = 0;
static int sdl_window_created = 0;

static const struct { const char *kw; TokenType type; } KEYWORDS[] = {
    {"var", TOK_VAR},
    {"ret", TOK_RETURN},
    {"out", TOK_PRINT},
    {"rep", TOK_LOOP_START},
    {"fun", TOK_FUN_START},
    {"if", TOK_IF_START},
    {"else", TOK_ELSE},
    {"end", TOK_END},
    {"keyPressed", TOK_KEY_PRESSED},
    {"keyDown", TOK_KEY_DOWN},
    {"add", TOK_ADD},
    {"stop", TOK_STOP},
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

        if (src[i + 1] == '=') {
            TokenType tt = TOK_UNKNOWN;
            switch (src[i]) {
                case '=': tt = TOK_EQUALS; break;
                case '!': tt = TOK_NOT_EQUALS; break;
                case '>': tt = TOK_MORE_EQUALS; break;
                case '<': tt = TOK_LESS_EQUALS; break;
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
            case '{': tt = TOK_LBRACE; break;
            case '}': tt = TOK_RBRACE; break;
            case '(': tt = TOK_LBRACKET; break;
            case ')': tt = TOK_RBRACKET; break;
            case '[': tt = TOK_LSQUARE; break;
            case ']': tt = TOK_RSQUARE; break;
            case ',': tt = TOK_COMMA; break;
            case '=': tt = TOK_ASSIGN; break;
            case '>': tt = TOK_MORE; break;
            case '<': tt = TOK_LESS; break;
            case '+': tt = TOK_PLUS; break;
            case '-': tt = TOK_MINUS; break;
            case '*': tt = TOK_MUL; break;
            case '/': tt = TOK_DIV; break;
            case '%': tt = TOK_MOD; break;
            case '.': tt = TOK_DOT; break;
            default: tt = TOK_UNKNOWN; break;
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

        i++;
        if (tokens[i].type != TOK_STRING) {
            fprintf(stderr, "Expected filename after 'add'\n");
            exit(1);
        }
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
        }
        else {
            fclose(f);
        }
    }
    if (out_idx < max_tokens) output[out_idx++].type = TOK_EOF;
    return out_idx;
}

static size_t find_block_end(Token tokens[], size_t token_count, size_t start) {
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

static size_t copy_block(Token tokens[], size_t start, size_t end, Token dst[]) {
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
        if (tokens[i].type != TOK_FUN_START) { i++; continue; }
        i++;

        if (tokens[i].type != TOK_IDENT) {
            fprintf(stderr, "Expected function name after 'fun'\n");
            exit(1);
        }

        Function *f = &functions[function_count];
        strncpy(f->name, tokens[i].text, MAX_NAME_LEN - 1);
        f->param_count = 0;
        i++;

        if (tokens[i].type == TOK_LBRACKET) {
            i++;
            while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                if (tokens[i].type != TOK_IDENT) {
                    fprintf(stderr, "Expected parameter name\n");
                    exit(1);
                }
                strncpy(f->param_names[f->param_count++], tokens[i].text, MAX_NAME_LEN - 1);
                i++;
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

static void write_indent(FILE *out, int indent) {
    for (int k = 0; k < indent; k++) fputs("    ", out);
}

static const char *map_builtin(const char *name) {
    if (!strcmp(name, "sin")) return "std::sin";
    if (!strcmp(name, "cos")) return "std::cos";
    if (!strcmp(name, "abs")) return "std::fabs";
    if (!strcmp(name, "min")) return "std::min<double>";
    if (!strcmp(name, "max")) return "std::max<double>";
    if (!strcmp(name, "lerp")) return "_lerp";
    if (!strcmp(name, "distance")) return "_distance";
    if (!strcmp(name, "mod")) return "std::fmod";
    if (!strcmp(name, "random")) return "_random";
    if (!strcmp(name, "sizeof")) return "sizeof";
    return NULL;
}

static const char *comp_op_str(TokenType op) {
    switch (op) {
        case TOK_EQUALS: return "==";
        case TOK_NOT_EQUALS: return "!=";
        case TOK_MORE: return ">";
        case TOK_LESS: return "<";
        case TOK_MORE_EQUALS: return ">=";
        case TOK_LESS_EQUALS: return "<=";
        default: return "?";
    }
}

static int is_builtin_call(Token tokens[], size_t i, const char *name) {
    return tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET && strcmp(tokens[i].text, name) == 0;
}

static size_t emit_expression(Token tokens[], size_t i, FILE *out);

static size_t emit_value(Token tokens[], size_t i, FILE *out) {
    if (tokens[i].type == TOK_MINUS || tokens[i].type == TOK_PLUS) {
        fputc(tokens[i].type == TOK_MINUS ? '-' : '+', out);
        return emit_value(tokens, i + 1, out);
    }

    if (tokens[i].type == TOK_NUMBER) {
        fputs(tokens[i].text, out);
        return i + 1;
    }

    if (tokens[i].type == TOK_IDENT) {
        if (tokens[i + 1].type == TOK_LBRACKET && strcmp(tokens[i].text, "random") == 0) {
            fputs("_random(", out);
            i += 2;
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
            fputs(", ", out);
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_RBRACKET) i++;
            fputc(')', out);
            return i;
        }
        if (tokens[i + 1].type == TOK_LBRACKET) {
            const char *mapped = map_builtin(tokens[i].text);
            if (mapped) {
                fprintf(out, "%s(", mapped);
                i += 2;
                int first = 1;
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (!first) fputs(", ", out);
                    first = 0;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
                fputc(')', out);
                return i;
            }
        }
        if (tokens[i + 1].type == TOK_LSQUARE) {
            fprintf(out, "%s[(int)(", tokens[i].text);
            i += 2;
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_RSQUARE) i++;
            fputs(")]", out);
            return i;
        }
        if (tokens[i + 1].type == TOK_DOT && tokens[i + 2].type == TOK_IDENT && strcmp(tokens[i + 2].text, "len") == 0) {
            fprintf(out, "%s.size()", tokens[i].text);
            return i + 3;
        }
        fputs(tokens[i].text, out);
        return i + 1;
    }

    if (tokens[i].type == TOK_LBRACKET) {
        fputc('(', out);
        i = emit_expression(tokens, i + 1, out);
        if (tokens[i].type == TOK_RBRACKET) i++;
        fputc(')', out);
        return i;
    }

    fprintf(stderr, "Expected value in expression: got type %d text '%s'\n", tokens[i].type, tokens[i].text);
    exit(1);
}

static size_t emit_expression(Token tokens[], size_t i, FILE *out) {
    i = emit_value(tokens, i, out);
    while (tokens[i].type == TOK_PLUS || tokens[i].type == TOK_MINUS || tokens[i].type == TOK_MUL || tokens[i].type == TOK_DIV || tokens[i].type == TOK_MOD) {
        const char *op = tokens[i].type == TOK_PLUS ? " + " : tokens[i].type == TOK_MINUS ? " - " : tokens[i].type == TOK_MUL ? " * " : tokens[i].type == TOK_DIV ? " / " : " % ";
        fputs(op, out);
        i++;
        i = emit_value(tokens, i, out);
    }
    return i;
}

static size_t parse_call_args(Token tokens[], size_t i, char bufs[][ARG_BUF_LEN], int max_args) {
    for (int a = 0; a < max_args; a++) bufs[a][0] = '\0';

    int arg = 0;
    while (arg < max_args && tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
        char *buf = bufs[arg];
        size_t bi = 0;

        if (tokens[i].type == TOK_STRING) {
            bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "\"%s\"", tokens[i].text);
            i++;
        }
        else {
            while (tokens[i].type != TOK_COMMA && tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                const char *part = NULL;
                switch (tokens[i].type) {
                    case TOK_NUMBER:
                    case TOK_IDENT: part = tokens[i].text; break;
                    case TOK_PLUS: part = " + "; break;
                    case TOK_MINUS: part = " - "; break;
                    case TOK_MUL: part = " * "; break;
                    case TOK_DIV: part = " / "; break;
                    case TOK_MOD: part = " % "; break;
                    default: break;
                }
                if (part) bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "%s", part);
                i++;
            }
        }
        if (tokens[i].type == TOK_COMMA) i++;
        arg++;
    }
    if (tokens[i].type == TOK_RBRACKET) i++;
    return i;
}

static size_t emit_call_args_inline(Token tokens[], size_t i, FILE *out) {
    int first = 1;
    while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
        if (!first) fputs(", ", out);
        first = 0;
        i = emit_expression(tokens, i, out);
        if (tokens[i].type == TOK_COMMA) i++;
    }
    fputc(')', out);
    if (tokens[i].type == TOK_RBRACKET) i++;
    return i;
}

static void emit_string_stream(const char *str, FILE *out) {
    int first = 1;
    size_t j = 0;
    while (str[j] != '\0') {
        if (str[j] == '{') {
            j++;
            char vname[MAX_NAME_LEN];
            size_t vi = 0;
            while (str[j] != '}' && str[j] != '\0' && vi + 1 < MAX_NAME_LEN)
                vname[vi++] = str[j++];
            vname[vi] = '\0';
            if (str[j] == '}') j++;
            if (!first) fputs(" << ", out);
            fputs(vname, out);
        }
        else {
            if (!first) fputs(" << ", out);
            fputc('"', out);
            while (str[j] != '\0' && str[j] != '{') {
                if (str[j] == '"') fputs("\\\"", out);
                else fputc(str[j], out);
                j++;
            }
            fputc('"', out);
        }
        first = 0;
    }
    if (first) fputs("\"\"", out);
}

typedef enum { RT_VOID, RT_DOUBLE, RT_STRING } RetType;

static RetType detect_return_type(Token tokens[], size_t token_count) {
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_RETURN) {
            if (i + 1 < token_count && tokens[i + 1].type == TOK_STRING)
                return RT_STRING;
            return RT_DOUBLE;
        }
    }
    return RT_VOID;
}

static int param_is_array(const char *param_name, Token tokens[], size_t token_count) {
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_IDENT && strcmp(tokens[i].text, param_name) == 0 && i + 1 < token_count && tokens[i + 1].type == TOK_LSQUARE) {
            return 1;
        }
        if (tokens[i].type == TOK_STRING) {
            char search[MAX_STRING_LEN + 4];
            snprintf(search, sizeof(search), "%s[", param_name);
            if (strstr(tokens[i].text, search)) return 1;
        }
    }
    return 0;
}

static void emit_func_signature(FILE *out, Function *f) {
    RetType rt = detect_return_type(f->tokens, f->token_count);
    const char *ret = rt == RT_STRING ? "std::string" : rt == RT_DOUBLE ? "double" : "void";
    fprintf(out, "%s %s(", ret, f->name);
    for (size_t pi = 0; pi < f->param_count; pi++) {
        if (pi > 0) fputs(", ", out);
        if (param_is_array(f->param_names[pi], f->tokens, f->token_count)) fprintf(out, "_KnTable& %s", f->param_names[pi]);
        else fprintf(out, "double %s", f->param_names[pi]);
    }
    fputc(')', out);
}

void convert_tokens_to_cpp(Token tokens[], size_t token_count, FILE *out, int indent, int is_main) {
    size_t i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (is_builtin_call(tokens, i, "createWindow")) {
            i += 2;
            char b[3][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 3);
            write_indent(out, indent);
            fputs("SDL_Init(SDL_INIT_VIDEO);\n", out);
            write_indent(out, indent);
            fprintf(out, "_window = SDL_CreateWindow(%s, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, %s, %s, SDL_WINDOW_SHOWN);\n", b[2], b[0], b[1]);
            write_indent(out, indent);
            fputs("_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);\n", out);
            sdl_window_created = 1;
            continue;
        }

        if (is_builtin_call(tokens, i, "clearScreen")) {
            i += 2;
            write_indent(out, indent);
            fputs("SDL_SetRenderDrawColor(_renderer, ", out);
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
            fputs(", ", out);
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
            fputs(", ", out);
            i = emit_expression(tokens, i, out);
            fputs(", 255);\n", out);
            if (tokens[i].type == TOK_RBRACKET) i++;
            write_indent(out, indent);
            fputs("SDL_RenderClear(_renderer);\n", out);
            continue;
        }

        if (is_builtin_call(tokens, i, "drawPixel")) {
            i += 2;
            char b[5][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 5);
            write_indent(out, indent);
            fprintf(out, "SDL_SetRenderDrawColor(_renderer, %s, %s, %s, 255);\n", b[2], b[3], b[4]);
            write_indent(out, indent);
            fprintf(out, "SDL_RenderDrawPoint(_renderer, %s, %s);\n", b[0], b[1]);
            continue;
        }

        if (is_builtin_call(tokens, i, "drawSquare")) {
            i += 2;
            char b[6][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 6);
            write_indent(out, indent);
            fprintf(out, "{ SDL_SetRenderDrawColor(_renderer, %s, %s, %s, 255);\n", b[3], b[4], b[5]);
            write_indent(out, indent);
            fprintf(out, "  SDL_Rect _sq = {(int)(%s),(int)(%s),(int)(%s),(int)(%s)};\n", b[0], b[1], b[2], b[2]);
            write_indent(out, indent);
            fputs("  SDL_RenderFillRect(_renderer, &_sq); }\n", out);
            continue;
        }

        if (is_builtin_call(tokens, i, "drawCircle")) {
            i += 2;
            char b[6][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 6);
            write_indent(out, indent);
            fprintf(out, "_drawCircle(_renderer, (int)(%s),(int)(%s),(int)(%s),(int)(%s),(int)(%s),(int)(%s));\n", b[0], b[1], b[2], b[3], b[4], b[5]);
            continue;
        }

        if (tokens[i].type == TOK_STOP) {
            i++;
            write_indent(out, indent);
            fputs("exit(0);\n", out);
            continue;
        }

        if (tokens[i].type == TOK_VAR) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i + 1].text);
            i += 3;
            write_indent(out, indent);

            if (tokens[i].type == TOK_LSQUARE) {
                i++;
                fprintf(out, "_KnTable %s = {", name);
                int first = 1;
                while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                    if (!first) fputs(", ", out);
                    first = 0;
                    if (tokens[i].type == TOK_STRING) {
                        fprintf(out, "\"%s\"", tokens[i].text);
                        i++;
                    } else {
                        i = emit_expression(tokens, i, out);
                    }
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                if (tokens[i].type == TOK_RSQUARE) i++;
                fputs("};\n", out);
                continue;
            }

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET && !map_builtin(tokens[i].text)) {
                const char *fn = tokens[i].text;
                i += 2;
                fprintf(out, "auto %s = %s(", name, fn);
                i = emit_call_args_inline(tokens, i, out);
                fputs(";\n", out);
                continue;
            }

            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "std::string %s = \"%s\";\n", name, tokens[i].text);
                i++;
                continue;
            }

            fprintf(out, "double %s = ", name);
            i = emit_expression(tokens, i, out);
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LSQUARE) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i].text);
            i += 2;
            write_indent(out, indent);
            fprintf(out, "%s[(int)(", name);
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_RSQUARE) i++;
            if (tokens[i].type == TOK_ASSIGN) i++;
            fputs(")] = ", out);
            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "\"%s\"", tokens[i].text);
                i++;
            }
            else {
                i = emit_expression(tokens, i, out);
            }
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_ASSIGN) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i].text);
            i += 2;
            write_indent(out, indent);

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET && !map_builtin(tokens[i].text)) {
                const char *fn = tokens[i].text;
                i += 2;
                fprintf(out, "%s = %s(", name, fn);
                i = emit_call_args_inline(tokens, i, out);
                fputs(";\n", out);
                continue;
            }

            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "%s = \"%s\";\n", name, tokens[i].text);
                i++;
                continue;
            }

            fprintf(out, "%s = ", name);
            i = emit_expression(tokens, i, out);
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
            const char *m = map_builtin(tokens[i].text);
            const char *fn = m ? m : tokens[i].text;
            i += 2;
            write_indent(out, indent);
            fprintf(out, "%s(", fn);
            i = emit_call_args_inline(tokens, i, out);
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_PRINT || tokens[i].type == TOK_PRINTL) {
            int is_printl = (tokens[i].type == TOK_PRINTL);
            i++;
            write_indent(out, indent);
            fputs("std::cout << std::fixed << std::setprecision(1) << ", out);
            if (tokens[i].type == TOK_STRING) {
                emit_string_stream(tokens[i].text, out);
                i++;
            } else {
                i = emit_expression(tokens, i, out);
            }
            if (is_printl) fputs(" << std::endl", out);
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IF_START) {
            i++;
            write_indent(out, indent);
            fputs("if (", out);

            if (tokens[i].type == TOK_KEY_PRESSED || tokens[i].type == TOK_KEY_DOWN) {
                const char *fn = (tokens[i].type == TOK_KEY_PRESSED) ? "_key_pressed" : "_key_down";
                i++;
                if (tokens[i].type == TOK_LBRACKET) i++;
                if (tokens[i].type == TOK_STRING) {
                    fprintf(out, "%s(\"%s\")", fn, tokens[i].text);
                    i++;
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
            }
            else {
                if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
                    const char *fn = tokens[i].text;
                    i += 2;
                    fprintf(out, "%s(", fn);
                    i = emit_call_args_inline(tokens, i, out);
                } else {
                    i = emit_expression(tokens, i, out);
                }
                TokenType op = tokens[i].type;
                if (op == TOK_EQUALS || op == TOK_MORE || op == TOK_LESS || op == TOK_NOT_EQUALS || op == TOK_MORE_EQUALS || op == TOK_LESS_EQUALS) {
                    fprintf(out, " %s ", comp_op_str(op));
                    i++;
                    i = emit_expression(tokens, i, out);
                } else {
                    fputs(" != 0", out);
                }
            }

            fputs(") {\n", out);

            while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) i++;
            if (tokens[i].type == TOK_LBRACE) i++;

            size_t end = find_block_end(tokens, token_count, i);
            Token block[MAX_TOKENS];
            size_t bc = copy_block(tokens, i, end, block);
            convert_tokens_to_cpp(block, bc, out, indent + 1, is_main);
            i = end + 1;

            if (i < token_count && tokens[i].type == TOK_ELSE) {
                i++;
                write_indent(out, indent);
                fputs("} else {\n", out);
                if (tokens[i].type == TOK_LBRACE) i++;
                size_t e_end = find_block_end(tokens, token_count, i);
                bc = copy_block(tokens, i, e_end, block);
                convert_tokens_to_cpp(block, bc, out, indent + 1, is_main);
                i = e_end + 1;
            }

            write_indent(out, indent);
            fputs("}\n", out);
            continue;
        }

        if (tokens[i].type == TOK_LOOP_START) {
            i++;
            size_t expr_start = i;
            while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) i++;
            size_t expr_end = i;
            if (tokens[i].type == TOK_LBRACE) i++;

            int is_simple_ident = (expr_end - expr_start == 1 && tokens[expr_start].type == TOK_IDENT);

            size_t loop_end = find_block_end(tokens, token_count, i);
            Token loop_tokens[MAX_TOKENS];
            size_t lc = copy_block(tokens, i, loop_end, loop_tokens);

            if (is_main && sdl_window_created) {
                write_indent(out, indent); fputs("{\n", out);
                write_indent(out, indent + 1); fputs("bool _running = true;\n", out);
                write_indent(out, indent + 1); fputs("SDL_Event _event;\n", out);
                write_indent(out, indent + 1); fputs("_last_frame_time = SDL_GetTicks();\n", out);
                write_indent(out, indent + 1); fputs("while (_running) {\n", out);
                write_indent(out, indent + 2); fputs("Uint32 _current_time = SDL_GetTicks();\n", out);
                write_indent(out, indent + 2); fputs("deltaTime = (_current_time - _last_frame_time) / 1000.0;\n", out);
                write_indent(out, indent + 2); fputs("_last_frame_time = _current_time;\n", out);
                write_indent(out, indent + 2); fputs("memcpy(_prev_key_state, _key_state, sizeof(_key_state));\n", out);
                write_indent(out, indent + 2); fputs("while (SDL_PollEvent(&_event)) {\n", out);
                write_indent(out, indent + 3); fputs("if (_event.type == SDL_QUIT) _running = false;\n", out);
                write_indent(out, indent + 3); fputs("if (_event.type == SDL_KEYDOWN) _key_state[_event.key.keysym.sym % 512] = 1;\n", out);
                write_indent(out, indent + 3); fputs("if (_event.type == SDL_KEYUP) _key_state[_event.key.keysym.sym % 512] = 0;\n", out);
                write_indent(out, indent + 2); fputs("}\n", out);
                convert_tokens_to_cpp(loop_tokens, lc, out, indent + 2, 0);
                write_indent(out, indent + 2); fputs("SDL_RenderPresent(_renderer);\n", out);
                write_indent(out, indent + 1); fputs("}\n", out);
                write_indent(out, indent); fputs("}\n", out);
            } else if (is_simple_ident) {
                const char *c = tokens[expr_start].text;
                write_indent(out, indent);
                fprintf(out, "{ double _rep_%s = %s; for (%s = 0; %s < _rep_%s; %s++) {\n", c, c, c, c, c, c);
                convert_tokens_to_cpp(loop_tokens, lc, out, indent + 1, is_main);
                write_indent(out, indent);
                fputs("} }\n", out);
            } else {
                static int loop_idx = 0;
                char expr[256] = "";
                size_t el = 0;
                for (size_t ei = expr_start; ei < expr_end; ei++) {
                    Token *et = &tokens[ei];
                    const char *part = "";
                    if (et->type == TOK_IDENT || et->type == TOK_NUMBER) part = et->text;
                    else if (et->type == TOK_PLUS) part = " + ";
                    else if (et->type == TOK_MINUS) part = " - ";
                    else if (et->type == TOK_MUL) part = " * ";
                    else if (et->type == TOK_DIV) part = " / ";
                    else if (et->type == TOK_MOD) part = " % ";
                    else if (et->type == TOK_DOT && ei + 1 < expr_end && tokens[ei + 1].type == TOK_IDENT && strcmp(tokens[ei + 1].text, "len") == 0) { part = ".size()"; ei++; }
                    el += snprintf(expr + el, sizeof(expr) - el, "%s", part);
                }
                int idx = loop_idx++;
                write_indent(out, indent);
                fprintf(out, "{ double _rep_%d = %s; for (double _loop_%d = 0; _loop_%d < _rep_%d; _loop_%d++) {\n", idx, expr, idx, idx, idx, idx);
                convert_tokens_to_cpp(loop_tokens, lc, out, indent + 1, is_main);
                write_indent(out, indent);
                fputs("} }\n", out);
            }

            i = loop_end + 1;
            continue;
        }

        if (tokens[i].type == TOK_RETURN) {
            i++;
            write_indent(out, indent);
            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "return \"%s\";\n", tokens[i].text);
                i++;
            } else {
                fputs("return ", out);
                i = emit_expression(tokens, i, out);
                fputs(";\n", out);
            }
            continue;
        }

        if (tokens[i].type == TOK_END) return;

        fprintf(stderr,
            "Unknown token in converter at position %zu, type: %d, text: '%s'\n",
            i, tokens[i].type, tokens[i].text);
        exit(1);
    }
}

void convert_to_cpp(Token tokens[], size_t token_count, const char *output_path) {
    sdl_window_created = 0;
    parse_functions(tokens, token_count);

    FILE *out = fopen(output_path, "w");
    if (!out) { perror("fopen"); exit(1); }

    fputs(
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <cstring>\n"
        "#include <vector>\n"
        "#include <random>\n"
        "#include <iomanip>\n"
        "#include <cmath>\n",
        out);
#if HAVE_SDL
    fputs("#include <SDL2/SDL.h>\n", out);
#endif

    fputs(
        "\n"
        "struct _KnVal {\n"
        "    bool _s; double _n; std::string _t;\n"
        "    _KnVal(double v):_s(false),_n(v){}\n"
        "    _KnVal(const char* v):_s(true),_n(0),_t(v){}\n"
        "    _KnVal(const std::string& v):_s(true),_n(0),_t(v){}\n"
        "    operator double() const { return _n; }\n"
        "    operator std::string() const { return _t; }\n"
        "    friend std::ostream& operator<<(std::ostream& os, const _KnVal& v) {\n"
        "        if (v._s) return os << v._t;\n"
        "        return os << v._n;\n"
        "    }\n"
        "};\n"
        "using _KnTable = std::vector<_KnVal>;\n\n"
        "std::mt19937 _rng(std::random_device{}());\n"
        "double _random(double min, double max) {\n"
        "    std::uniform_real_distribution<double> dist(min, max);\n"
        "    return dist(_rng);\n"
        "}\n"
        "double _lerp(double a, double b, double t) { return a + (b - a) * t; }\n"
        "double _distance(double x1, double y1, double x2, double y2) {\n"
        "    return std::sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));\n"
        "}\n\n",
        out);

#if HAVE_SDL
    fputs(
        "SDL_Window *_window = nullptr;\n"
        "SDL_Renderer *_renderer = nullptr;\n"
        "int _key_state[512] = {0};\n"
        "int _prev_key_state[512] = {0};\n"
        "double deltaTime = 0.0;\n"
        "Uint32 _last_frame_time = 0;\n\n"
        "bool _key_pressed(const char *key_name) {\n"
        "    SDL_Keycode kc = SDL_GetKeyFromName(key_name);\n"
        "    if (kc == SDLK_UNKNOWN) return false;\n"
        "    int idx = kc % 512;\n"
        "    return _key_state[idx] && !_prev_key_state[idx];\n"
        "}\n\n"
        "bool _key_down(const char *key_name) {\n"
        "    SDL_Keycode kc = SDL_GetKeyFromName(key_name);\n"
        "    if (kc == SDLK_UNKNOWN) return false;\n"
        "    int idx = kc % 512;\n"
        "    return _key_state[idx];\n"
        "}\n\n"
        "void _drawCircle(SDL_Renderer *r, int cx, int cy, int radius,\n"
        "                 int red, int green, int blue) {\n"
        "    SDL_SetRenderDrawColor(r, red, green, blue, 255);\n"
        "    int x = radius, y = 0, err = 0;\n"
        "    while (x >= y) {\n"
        "        SDL_RenderDrawPoint(r,cx+x,cy+y); SDL_RenderDrawPoint(r,cx+y,cy+x);\n"
        "        SDL_RenderDrawPoint(r,cx-y,cy+x); SDL_RenderDrawPoint(r,cx-x,cy+y);\n"
        "        SDL_RenderDrawPoint(r,cx-x,cy-y); SDL_RenderDrawPoint(r,cx-y,cy-x);\n"
        "        SDL_RenderDrawPoint(r,cx+y,cy-x); SDL_RenderDrawPoint(r,cx+x,cy-y);\n"
        "        if (err <= 0) { y++; err += 2*y+1; }\n"
        "        else { x--; err -= 2*x+1; }\n"
        "    }\n"
        "}\n\n",
        out);
#endif

    for (size_t fi = 0; fi < function_count; fi++) {
        if (strcmp(functions[fi].name, "main") == 0) continue;
        emit_func_signature(out, &functions[fi]);
        fputs(";\n", out);
    }
    if (function_count > 1) fputc('\n', out);

    for (size_t fi = 0; fi < function_count; fi++) {
        Function *f = &functions[fi];
        if (strcmp(f->name, "main") == 0) continue;
        emit_func_signature(out, f);
        fputs(" {\n", out);
        convert_tokens_to_cpp(f->tokens, f->token_count, out, 1, 0);
        fputs("}\n\n", out);
    }

    Function *main_func = get_function("main");
    if (main_func) {
        fputs("int main() {\n", out);
        convert_tokens_to_cpp(main_func->tokens, main_func->token_count, out, 1, 1);
#if HAVE_SDL
        if (sdl_window_created) {
            fputs(
                "    SDL_DestroyRenderer(_renderer);\n"
                "    SDL_DestroyWindow(_window);\n"
                "    SDL_Quit();\n",
                out);
        }
#endif
        fputs("    return 0;\n}\n", out);
    }

    fclose(out);
}

int main(int argc, char **argv) {
    int compile_only = 0;
    int remove_cpp   = 0;
    char *input_file = NULL;

    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--version") == 0) {
            printf("kinnie " KINNIE_VERSION "\n");
            return 0;
        }
        else if (strcmp(argv[a], "--compile") == 0) {
            compile_only = 1;
        }
        else if (strcmp(argv[a], "--remcpp") == 0) {
            remove_cpp = 1;
        }
        else {
            input_file = argv[a];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Usage: %s [--version] [--compile] [--remcpp] file.kn\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(input_file, "rb");
    if (!f) { perror("fopen"); return 1; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0) {
        fprintf(stderr, "The file is empty\n");
        fclose(f);
        return 1;
    }

    char *source = malloc(size + 1);
    if (!source) { perror("malloc"); fclose(f); return 1; }
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    Token  tokens[MAX_TOKENS];
    size_t token_count = tokenize(source, tokens);

    Token expanded[MAX_EXPANDED_TOKENS];
    Token *final_tokens = tokens;
    size_t final_count = token_count;
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_ADD) {
            memset(expanded, 0, sizeof(expanded));
            final_count = process_includes(tokens, token_count, expanded, MAX_EXPANDED_TOKENS);
            final_tokens = expanded;
            break;
        }
    }

#if HAVE_SDL == 0
    fprintf(stderr, "Warning: SDL2 not found. Install it with: sudo apt install libsdl2-dev\n");
#endif

    char output_path[256];
    strcpy(output_path, input_file);
    size_t len = strlen(output_path);
    if (len > 3 && strcmp(output_path + len - 3, ".kn") == 0)
        strcpy(output_path + len - 3, ".cpp");
    else
        strcat(output_path, ".cpp");
    convert_to_cpp(final_tokens, final_count, output_path);

    char basename[256];
    strcpy(basename, output_path);
    len = strlen(basename);
    if (len > 4 && strcmp(basename + len - 4, ".cpp") == 0)
        basename[len - 4] = '\0';

    char command[512];
    if (compile_only) {
        snprintf(command, sizeof(command), "g++ -std=c++17 %s -o %s $(pkg-config --cflags --libs sdl2 2>/dev/null)", output_path, basename);
        fprintf(stderr, "Compiling...\n");
    }
    else {
        snprintf(command, sizeof(command), "g++ -std=c++17 %s -o %s $(pkg-config --cflags --libs sdl2 2>/dev/null) && ./%s", output_path, basename, basename);
        fprintf(stderr, "Compiling and running...\n");
    }
    system(command);

    if (remove_cpp) remove(output_path);

    free(source);
    return 0;
}
