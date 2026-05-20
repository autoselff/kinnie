#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef __has_include
    #if __has_include(<SDL2/SDL.h>)
        #include <SDL2/SDL.h>
        #define HAVE_SDL 1
    #else
        #define HAVE_SDL 0
    #endif
#else
    #define HAVE_SDL 0
#endif

#define MAX_TOKENS 2048
#define MAX_EXPANDED_TOKENS 8192
#define MAX_FUNCTIONS 64
#define MAX_NAME_LEN 32
#define MAX_STRING_LEN 128
#define MAX_FUNC_PARAMS 16

typedef enum {
    TOK_VAR,
    TOK_PRINT,
    TOK_PRINTL,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_ASSIGN,
    TOK_MORE,
    TOK_LESS,
    TOK_EQUALS,
    TOK_NOT_EQUALS,
    TOK_MORE_EQUALS,
    TOK_LESS_EQUALS,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_EOF,
    TOK_MOD,
    TOK_LOOP_START,
    TOK_FUN_START,
    TOK_IF_START,
    TOK_ELSE,
    TOK_END,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_RETURN,
    TOK_KEY_PRESSED,
    TOK_KEY_DOWN,
    TOK_ADD,
    TOK_LSQUARE,
    TOK_RSQUARE,
    TOK_DOT,
    TOK_STOP,
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

Function functions[MAX_FUNCTIONS];
size_t function_count = 0;

size_t tokenize(const char *src, Token tokens[]) {
    size_t i = 0, t = 0;
    while (src[i]) {
        if (isspace(src[i])) {
            i++;
            continue;
        }
        if (src[i] == '/' && src[i+1] == '/') {
            while (src[i] && src[i] != '\n') i++;
            if (src[i] == '\n') i++;
            continue;
        }
        if (src[i] == '"') {
            i++;
            size_t start = i;
            while (src[i] && src[i] != '"') i++;

            size_t len = i - start;
            strncpy(tokens[t].text, src + start, len);
            tokens[t].text[len] = 0;
            tokens[t].type = TOK_STRING;
            if (src[i] == '"') i++;
            t++;
            continue;
        }
        if (isdigit(src[i]) || (src[i] == '.' && isdigit(src[i+1]))) {
            size_t start = i;
            while (isdigit(src[i])) i++;
            if (src[i] == '.' && isdigit(src[i+1])) {
                i++;
                while (isdigit(src[i])) i++;
            }

            size_t len = i - start;
            strncpy(tokens[t].text, src + start, len);
            tokens[t].text[len] = 0;
            tokens[t].type = TOK_NUMBER;
            t++;
            continue;
        }
        if (isalpha(src[i])) {
            size_t start = i;
            while (isalnum(src[i])) i++;

            size_t len = i - start;
            strncpy(tokens[t].text, src + start, len);
            tokens[t].text[len] = 0;

            if (strcmp(tokens[t].text, "var") == 0)
                tokens[t].type = TOK_VAR;
            else if (strcmp(tokens[t].text, "ret") == 0)
                tokens[t].type = TOK_RETURN;
            else if (strcmp(tokens[t].text, "out") == 0)
                tokens[t].type = TOK_PRINT;
            else if (strcmp(tokens[t].text, "rep") == 0)
                tokens[t].type = TOK_LOOP_START;
            else if (strcmp(tokens[t].text, "fun") == 0)
                tokens[t].type = TOK_FUN_START;
            else if (strcmp(tokens[t].text, "if") == 0)
                tokens[t].type = TOK_IF_START;
            else if (strcmp(tokens[t].text, "else") == 0)
                tokens[t].type = TOK_ELSE;
            else if (strcmp(tokens[t].text, "end") == 0)
                tokens[t].type = TOK_END;
            else if (strcmp(tokens[t].text, "keyPressed") == 0)
                tokens[t].type = TOK_KEY_PRESSED;
            else if (strcmp(tokens[t].text, "keyDown") == 0)
                tokens[t].type = TOK_KEY_DOWN;
            else if (strcmp(tokens[t].text, "add") == 0)
                tokens[t].type = TOK_ADD;
            else if (strcmp(tokens[t].text, "stop") == 0)
                tokens[t].type = TOK_STOP;
            else
                tokens[t].type = TOK_IDENT;

            t++;
            continue;
        }
        if (src[i] == '{') {
            tokens[t++].type = TOK_LBRACE;
            i++;
            continue;
        }
        if (src[i] == '}') {
            tokens[t++].type = TOK_RBRACE;
            i++;
            continue;
        }
        if (src[i] == '(') {
            tokens[t++].type = TOK_LBRACKET;
            i++;
            continue;
        }
        if (src[i] == ')') {
            tokens[t++].type = TOK_RBRACKET;
            i++;
            continue;
        }
        if (src[i] == '[') {
            tokens[t++].type = TOK_LSQUARE;
            i++;
            continue;
        }
        if (src[i] == ']') {
            tokens[t++].type = TOK_RSQUARE;
            i++;
            continue;
        }
        if (src[i] == ',') {
            tokens[t++].type = TOK_COMMA;
            i++;
            continue;
        }
        if (src[i] == '=') {
            if (src[i + 1] == '=') {
                tokens[t++].type = TOK_EQUALS;
                i += 2;
            } else {
                tokens[t++].type = TOK_ASSIGN;
                i++;
            }
            continue;
        }
        if (src[i] == '!') {
            if (src[i + 1] == '=') {
                tokens[t++].type = TOK_NOT_EQUALS;
                i += 2;
            } else {
                tokens[t++].type = TOK_UNKNOWN;
                i++;
            }
            continue;
        }
        if (src[i] == '>') {
            if (src[i + 1] == '=') {
                tokens[t++].type = TOK_MORE_EQUALS;
                i += 2;
            } else {
                tokens[t++].type = TOK_MORE;
                i++;
            }
            continue;
        }
        if (src[i] == '<') {
            if (src[i + 1] == '=') {
                tokens[t++].type = TOK_LESS_EQUALS;
                i += 2;
            } else {
                tokens[t++].type = TOK_LESS;
                i++;
            }
            continue;
        }
        switch (src[i]) {
            case '+': tokens[t++].type = TOK_PLUS; break;
            case '-': tokens[t++].type = TOK_MINUS; break;
            case '*': tokens[t++].type = TOK_MUL; break;
            case '/': tokens[t++].type = TOK_DIV; break;
            case '%': tokens[t++].type = TOK_MOD; break;
            case '.': tokens[t++].type = TOK_DOT; break;
            default:  tokens[t++].type = TOK_UNKNOWN; break;
        }
        i++;
    }
    tokens[t].type = TOK_EOF;
    return t;
}

Function *get_function(const char *name) {
    for (size_t i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0)
            return &functions[i];
    }
    return NULL;
}

size_t process_includes(Token tokens[], size_t token_count, Token output[], size_t max_tokens) {
    size_t out_idx = 0;
    size_t i = 0;

    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (tokens[i].type == TOK_ADD) {
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

                Token temp_tokens[MAX_TOKENS];
                size_t temp_count = tokenize(source, temp_tokens);

                for (size_t j = 0; j < temp_count && out_idx < max_tokens; j++) {
                    if (temp_tokens[j].type != TOK_EOF) {
                        output[out_idx++] = temp_tokens[j];
                    }
                }

                free(source);
            } else {
                fclose(f);
            }
        } else {
            if (out_idx < max_tokens) {
                output[out_idx++] = tokens[i];
            }
            i++;
        }
    }

    if (out_idx < max_tokens) {
        output[out_idx].type = TOK_EOF;
        out_idx++;
    }

    return out_idx;
}

void parse_functions(Token tokens[], size_t token_count) {
    memset(functions, 0, sizeof(functions));
    size_t i = 0;

    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (tokens[i].type == TOK_FUN_START) {
            i++;

            if (tokens[i].type != TOK_IDENT) {
                fprintf(stderr, "Expected function name after 'fun'\n");
                exit(1);
            }

            char func_name[MAX_NAME_LEN];
            strcpy(func_name, tokens[i].text);
            i++;

            functions[function_count].param_count = 0;

            if (tokens[i].type == TOK_LBRACKET) {
                i++;

                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (tokens[i].type == TOK_IDENT) {
                        strncpy(
                            functions[function_count].param_names[functions[function_count].param_count],
                            tokens[i].text,
                            MAX_NAME_LEN - 1
                        );
                        functions[function_count].param_count++;
                        i++;

                        if (tokens[i].type == TOK_COMMA) {
                            i++;
                        }
                    } else {
                        fprintf(stderr, "Expected parameter name\n");
                        exit(1);
                    }
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

            size_t fun_start = i;
            size_t fun_end = i;
            int depth = 1;

            while (fun_end < token_count && tokens[fun_end].type != TOK_EOF) {
                if (tokens[fun_end].type == TOK_LBRACE) depth++;
                if (tokens[fun_end].type == TOK_RBRACE) {
                    depth--;
                    if (depth == 0) break;
                }
                fun_end++;
            }

            strncpy(functions[function_count].name, func_name, MAX_NAME_LEN - 1);
            functions[function_count].token_count = 0;

            for (size_t j = fun_start; j < fun_end && functions[function_count].token_count < MAX_TOKENS - 1; j++) {
                functions[function_count].tokens[functions[function_count].token_count++] = tokens[j];
            }
            if (functions[function_count].token_count < MAX_TOKENS) {
                functions[function_count].tokens[functions[function_count].token_count].type = TOK_EOF;
            }


            function_count++;
            i = fun_end + 1;
            continue;
        }
        i++;
    }
}

static int sdl_window_created = 0;

static void write_indent(FILE *out, int indent) {
    for (int k = 0; k < indent; k++)
        fprintf(out, "    ");
}

static const char *map_builtin(const char *name) {
    if (strcmp(name, "sin")      == 0) return "std::sin";
    if (strcmp(name, "cos")      == 0) return "std::cos";
    if (strcmp(name, "abs")      == 0) return "std::fabs";
    if (strcmp(name, "min")      == 0) return "std::min<double>";
    if (strcmp(name, "max")      == 0) return "std::max<double>";
    if (strcmp(name, "lerp")     == 0) return "_lerp";
    if (strcmp(name, "distance") == 0) return "_distance";
    if (strcmp(name, "mod")      == 0) return "std::fmod";
    if (strcmp(name, "random")   == 0) return "_random";
    return NULL;
}

static size_t emit_expression(Token tokens[], size_t i, FILE *out);

static size_t emit_value(Token tokens[], size_t i, FILE *out) {
    if (tokens[i].type == TOK_MINUS || tokens[i].type == TOK_PLUS) {
        fprintf(out, "%c", tokens[i].type == TOK_MINUS ? '-' : '+');
        return emit_value(tokens, i + 1, out);
    }
    if (tokens[i].type == TOK_NUMBER) { fprintf(out, "%s", tokens[i].text); return i + 1; }
    if (tokens[i].type == TOK_IDENT) {
        if (tokens[i+1].type == TOK_LBRACKET && strcmp(tokens[i].text, "random") == 0) {
            fprintf(out, "_random(");
            i += 2;
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
            fprintf(out, ", ");
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_RBRACKET) i++;
            fprintf(out, ")");
            return i;
        }
        if (tokens[i+1].type == TOK_LBRACKET) {
            const char *mapped = map_builtin(tokens[i].text);
            if (mapped) {
                fprintf(out, "%s(", mapped);
                i += 2;
                int first = 1;
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (!first) fprintf(out, ", ");
                    first = 0;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
                fprintf(out, ")");
                return i;
            }
        }
        if (tokens[i+1].type == TOK_LSQUARE) {
            fprintf(out, "%s[(int)(", tokens[i].text);
            i += 2;
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_RSQUARE) i++;
            fprintf(out, ")]");
            return i;
        }
        if (tokens[i+1].type == TOK_DOT && i + 2 < 512 && tokens[i+2].type == TOK_IDENT && strcmp(tokens[i+2].text, "len") == 0) {
            fprintf(out, "%s.size()", tokens[i].text);
            return i + 3;
        }
        fprintf(out, "%s", tokens[i].text);
        return i + 1;
    }
    if (tokens[i].type == TOK_LBRACKET) {
        fprintf(out, "(");
        i++;
        i = emit_expression(tokens, i, out);
        if (tokens[i].type == TOK_RBRACKET) i++;
        fprintf(out, ")");
        return i;
    }
    fprintf(stderr, "Expected value in expression: got type %d text '%s'\n", tokens[i].type, tokens[i].text);
    exit(1);
}

static size_t emit_expression(Token tokens[], size_t i, FILE *out) {
    i = emit_value(tokens, i, out);
    while (tokens[i].type == TOK_PLUS  || tokens[i].type == TOK_MINUS ||
           tokens[i].type == TOK_MUL   || tokens[i].type == TOK_DIV   ||
           tokens[i].type == TOK_MOD) {
        const char *op =
            tokens[i].type == TOK_PLUS  ? " + " :
            tokens[i].type == TOK_MINUS ? " - " :
            tokens[i].type == TOK_MUL   ? " * " :
            tokens[i].type == TOK_DIV   ? " / " : " % ";
        fprintf(out, "%s", op);
        i++;
        i = emit_value(tokens, i, out);
    }
    return i;
}

static const char *comp_op_str(TokenType op) {
    switch (op) {
        case TOK_EQUALS:      return "==";
        case TOK_NOT_EQUALS:  return "!=";
        case TOK_MORE:        return ">";
        case TOK_LESS:        return "<";
        case TOK_MORE_EQUALS: return ">=";
        case TOK_LESS_EQUALS: return "<=";
        default:              return "?";
    }
}

typedef enum { RT_VOID, RT_DOUBLE, RT_STRING } RetType;

static int param_is_array(const char *param_name, Token tokens[], size_t token_count) {
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_IDENT && strcmp(tokens[i].text, param_name) == 0 &&
            i + 1 < token_count && tokens[i + 1].type == TOK_LSQUARE) {
            return 1;
        }
        if (tokens[i].type == TOK_STRING) {
            char search[MAX_STRING_LEN + 4];
            snprintf(search, sizeof(search), "%s[", param_name);
            if (strstr(tokens[i].text, search)) {
                return 1;
            }
        }
    }
    return 0;
}

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

static void emit_string_stream(const char *str, FILE *out) {
    size_t j = 0;
    int first = 1;
    while (str[j] != '\0') {
        if (str[j] == '{') {
            j++;
            char vname[MAX_NAME_LEN];
            size_t vi = 0;
            while (str[j] != '}' && str[j] != '\0') vname[vi++] = str[j++];
            vname[vi] = '\0';
            if (str[j] == '}') j++;
            if (!first) fprintf(out, " << ");
            fprintf(out, "%s", vname);
            first = 0;
        } else {
            if (!first) fprintf(out, " << ");
            fprintf(out, "\"");
            while (str[j] != '\0' && str[j] != '{') {
                if (str[j] == '"') fprintf(out, "\\\"");
                else               fprintf(out, "%c", str[j]);
                j++;
            }
            fprintf(out, "\"");
            first = 0;
        }
    }
    if (first) fprintf(out, "\"\"");
}

void convert_tokens_to_cpp(Token tokens[], size_t token_count, FILE *out, int indent, int is_main);

void convert_tokens_to_cpp(Token tokens[], size_t token_count, FILE *out, int indent, int is_main) {
    size_t i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
            strcmp(tokens[i].text, "createWindow") == 0) {
            i += 2;
            char w_buf[256] = "", h_buf[256] = "", t_buf[256] = "";
            size_t buf_i = 0;

            for (int arg = 0; arg < 3 && tokens[i].type != TOK_RBRACKET; arg++) {
                char *buf = (arg == 0 ? w_buf : arg == 1 ? h_buf : t_buf);
                buf_i = 0;

                if (arg == 2 && tokens[i].type == TOK_STRING) {
                    snprintf(buf, 256, "\"%s\"", tokens[i].text);
                    i++;
                } else {
                    while (tokens[i].type != TOK_COMMA && tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                        if (tokens[i].type == TOK_NUMBER) buf_i += snprintf(buf + buf_i, 256 - buf_i, "%s", tokens[i].text);
                        else if (tokens[i].type == TOK_IDENT) buf_i += snprintf(buf + buf_i, 256 - buf_i, "%s", tokens[i].text);
                        else if (tokens[i].type == TOK_PLUS) buf_i += snprintf(buf + buf_i, 256 - buf_i, " + ");
                        else if (tokens[i].type == TOK_MINUS) buf_i += snprintf(buf + buf_i, 256 - buf_i, " - ");
                        i++;
                    }
                }
                if (tokens[i].type == TOK_COMMA) i++;
            }

            write_indent(out, indent);
            fprintf(out, "SDL_Init(SDL_INIT_VIDEO);\n");
            write_indent(out, indent);
            fprintf(out, "_window = SDL_CreateWindow(%s, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, %s, %s, SDL_WINDOW_SHOWN);\n", t_buf, w_buf, h_buf);
            write_indent(out, indent);
            fprintf(out, "_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);\n");
            if (tokens[i].type == TOK_RBRACKET) i++;
            sdl_window_created = 1;
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
            strcmp(tokens[i].text, "clearScreen") == 0) {
            i += 2;
            write_indent(out, indent);
            fprintf(out, "SDL_SetRenderDrawColor(_renderer, ");
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
            fprintf(out, ", ");
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
            fprintf(out, ", ");
            i = emit_expression(tokens, i, out);
            fprintf(out, ", 255);\n");
            if (tokens[i].type == TOK_RBRACKET) i++;
            write_indent(out, indent);
            fprintf(out, "SDL_RenderClear(_renderer);\n");
            continue;
        }

        if (tokens[i].type == TOK_STOP) {
            i++;
            write_indent(out, indent);
            fprintf(out, "exit(0);\n");
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
            strcmp(tokens[i].text, "drawPixel") == 0) {
            i += 2;
            char x_buf[256] = "", y_buf[256] = "", r_buf[256] = "", g_buf[256] = "", b_buf[256] = "";
            size_t buf_i = 0;

            for (int arg = 0; arg < 5 && tokens[i].type != TOK_RBRACKET; arg++) {
                char *buf = (arg == 0 ? x_buf : arg == 1 ? y_buf : arg == 2 ? r_buf : arg == 3 ? g_buf : b_buf);
                buf_i = 0;

                while (tokens[i].type != TOK_COMMA && tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (tokens[i].type == TOK_NUMBER) buf_i += snprintf(buf + buf_i, 256 - buf_i, "%s", tokens[i].text);
                    else if (tokens[i].type == TOK_IDENT) buf_i += snprintf(buf + buf_i, 256 - buf_i, "%s", tokens[i].text);
                    else if (tokens[i].type == TOK_PLUS) buf_i += snprintf(buf + buf_i, 256 - buf_i, " + ");
                    else if (tokens[i].type == TOK_MINUS) buf_i += snprintf(buf + buf_i, 256 - buf_i, " - ");
                    else if (tokens[i].type == TOK_MUL) buf_i += snprintf(buf + buf_i, 256 - buf_i, " * ");
                    else if (tokens[i].type == TOK_DIV) buf_i += snprintf(buf + buf_i, 256 - buf_i, " / ");
                    i++;
                }
                if (tokens[i].type == TOK_COMMA) i++;
            }

            write_indent(out, indent);
            fprintf(out, "SDL_SetRenderDrawColor(_renderer, %s, %s, %s, 255);\n", r_buf, g_buf, b_buf);
            write_indent(out, indent);
            fprintf(out, "SDL_RenderDrawPoint(_renderer, %s, %s);\n", x_buf, y_buf);
            if (tokens[i].type == TOK_RBRACKET) i++;
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
            strcmp(tokens[i].text, "drawSquare") == 0) {
            i += 2;
            char x_buf[256]="", y_buf[256]="", s_buf[256]="", r_buf[256]="", g_buf[256]="", b_buf[256]="";
            size_t buf_i = 0;
            for (int arg = 0; arg < 6 && tokens[i].type != TOK_RBRACKET; arg++) {
                char *buf = (arg==0?x_buf:arg==1?y_buf:arg==2?s_buf:arg==3?r_buf:arg==4?g_buf:b_buf);
                buf_i = 0;
                while (tokens[i].type != TOK_COMMA && tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if      (tokens[i].type == TOK_NUMBER) buf_i += snprintf(buf+buf_i, 256-buf_i, "%s", tokens[i].text);
                    else if (tokens[i].type == TOK_IDENT)  buf_i += snprintf(buf+buf_i, 256-buf_i, "%s", tokens[i].text);
                    else if (tokens[i].type == TOK_PLUS)   buf_i += snprintf(buf+buf_i, 256-buf_i, " + ");
                    else if (tokens[i].type == TOK_MINUS)  buf_i += snprintf(buf+buf_i, 256-buf_i, " - ");
                    else if (tokens[i].type == TOK_MUL)    buf_i += snprintf(buf+buf_i, 256-buf_i, " * ");
                    else if (tokens[i].type == TOK_DIV)    buf_i += snprintf(buf+buf_i, 256-buf_i, " / ");
                    i++;
                }
                if (tokens[i].type == TOK_COMMA) i++;
            }
            write_indent(out, indent);
            fprintf(out, "{ SDL_SetRenderDrawColor(_renderer, %s, %s, %s, 255);\n", r_buf, g_buf, b_buf);
            write_indent(out, indent);
            fprintf(out, "  SDL_Rect _sq = {(int)(%s),(int)(%s),(int)(%s),(int)(%s)};\n", x_buf, y_buf, s_buf, s_buf);
            write_indent(out, indent);
            fprintf(out, "  SDL_RenderFillRect(_renderer, &_sq); }\n");
            if (tokens[i].type == TOK_RBRACKET) i++;
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
            strcmp(tokens[i].text, "drawCircle") == 0) {
            i += 2;
            char x_buf[256]="", y_buf[256]="", rad_buf[256]="", r_buf[256]="", g_buf[256]="", b_buf[256]="";
            size_t buf_i = 0;
            for (int arg = 0; arg < 6 && tokens[i].type != TOK_RBRACKET; arg++) {
                char *buf = (arg==0?x_buf:arg==1?y_buf:arg==2?rad_buf:arg==3?r_buf:arg==4?g_buf:b_buf);
                buf_i = 0;
                while (tokens[i].type != TOK_COMMA && tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if      (tokens[i].type == TOK_NUMBER) buf_i += snprintf(buf+buf_i, 256-buf_i, "%s", tokens[i].text);
                    else if (tokens[i].type == TOK_IDENT)  buf_i += snprintf(buf+buf_i, 256-buf_i, "%s", tokens[i].text);
                    else if (tokens[i].type == TOK_PLUS)   buf_i += snprintf(buf+buf_i, 256-buf_i, " + ");
                    else if (tokens[i].type == TOK_MINUS)  buf_i += snprintf(buf+buf_i, 256-buf_i, " - ");
                    else if (tokens[i].type == TOK_MUL)    buf_i += snprintf(buf+buf_i, 256-buf_i, " * ");
                    else if (tokens[i].type == TOK_DIV)    buf_i += snprintf(buf+buf_i, 256-buf_i, " / ");
                    i++;
                }
                if (tokens[i].type == TOK_COMMA) i++;
            }
            write_indent(out, indent);
            fprintf(out, "_drawCircle(_renderer, (int)(%s),(int)(%s),(int)(%s),(int)(%s),(int)(%s),(int)(%s));\n",
                    x_buf, y_buf, rad_buf, r_buf, g_buf, b_buf);
            if (tokens[i].type == TOK_RBRACKET) i++;
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
                    if (!first) fprintf(out, ", ");
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
                fprintf(out, "};\n");
                continue;
            }

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
                !map_builtin(tokens[i].text)) {
                const char *func_name = tokens[i].text;
                i += 2;
                fprintf(out, "auto %s = %s(", name, func_name);
                int first = 1;
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (!first) fprintf(out, ", ");
                    first = 0;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                i++;
                fprintf(out, ");\n");
                continue;
            }

            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "std::string %s = \"%s\";\n", name, tokens[i].text);
                i++;
            } else {
                fprintf(out, "double %s = ", name);
                i = emit_expression(tokens, i, out);
                fprintf(out, ";\n");
            }
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
            fprintf(out, ")] = ");
            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "\"%s\"", tokens[i].text);
                i++;
            } else {
                i = emit_expression(tokens, i, out);
            }
            fprintf(out, ";\n");
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_ASSIGN) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i].text);
            i += 2;

            write_indent(out, indent);

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET &&
                !map_builtin(tokens[i].text)) {
                const char *func_name = tokens[i].text;
                i += 2;
                fprintf(out, "%s = %s(", name, func_name);
                int first = 1;
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (!first) fprintf(out, ", ");
                    first = 0;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                i++;
                fprintf(out, ");\n");
                continue;
            }

            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "%s = \"%s\";\n", name, tokens[i].text);
                i++;
            } else {
                fprintf(out, "%s = ", name);
                i = emit_expression(tokens, i, out);
                fprintf(out, ";\n");
            }
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
            const char *_mb = map_builtin(tokens[i].text);
            const char *func_name = _mb ? _mb : tokens[i].text;
            i += 2;

            write_indent(out, indent);
            fprintf(out, "%s(", func_name);
            int first = 1;
            while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                if (!first) fprintf(out, ", ");
                first = 0;
                i = emit_expression(tokens, i, out);
                if (tokens[i].type == TOK_COMMA) i++;
            }
            i++;
            fprintf(out, ");\n");
            continue;
        }

        if (tokens[i].type == TOK_PRINT || tokens[i].type == TOK_PRINTL) {
            int is_printl = (tokens[i].type == TOK_PRINTL);
            i++;

            write_indent(out, indent);
            fprintf(out, "std::cout << std::fixed << std::setprecision(1) << ");

            if (tokens[i].type == TOK_STRING) {
                emit_string_stream(tokens[i].text, out);
                i++;
            } else {
                i = emit_expression(tokens, i, out);
            }

            if (is_printl) fprintf(out, " << std::endl");
            fprintf(out, ";\n");
            continue;
        }

        if (tokens[i].type == TOK_IF_START) {
            i++;

            write_indent(out, indent);
            fprintf(out, "if (");

            if (tokens[i].type == TOK_KEY_PRESSED || tokens[i].type == TOK_KEY_DOWN) {
                TokenType key_func = tokens[i].type;
                i++;
                if (tokens[i].type == TOK_LBRACKET) i++;
                if (tokens[i].type == TOK_STRING) {
                    if (key_func == TOK_KEY_PRESSED) {
                        fprintf(out, "_key_pressed(\"%s\")", tokens[i].text);
                    } else {
                        fprintf(out, "_key_down(\"%s\")", tokens[i].text);
                    }
                    i++;
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
            } else {
                if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
                    char func_name[MAX_NAME_LEN];
                    strcpy(func_name, tokens[i].text);
                    i += 2;
                    fprintf(out, "%s(", func_name);
                    int first = 1;
                    while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                        if (!first) fprintf(out, ", ");
                        first = 0;
                        i = emit_expression(tokens, i, out);
                        if (tokens[i].type == TOK_COMMA) i++;
                    }
                    fprintf(out, ")");
                    if (tokens[i].type == TOK_RBRACKET) i++;
                } else {
                    i = emit_expression(tokens, i, out);
                }
                TokenType op = tokens[i].type;
                if (op == TOK_EQUALS || op == TOK_MORE || op == TOK_LESS ||
                    op == TOK_NOT_EQUALS || op == TOK_MORE_EQUALS || op == TOK_LESS_EQUALS) {
                    fprintf(out, " %s ", comp_op_str(op));
                    i++;
                    i = emit_expression(tokens, i, out);
                } else {
                    fprintf(out, " != 0");
                }
            }

            fprintf(out, ") {\n");

            while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) {
                i++;
            }
            if (tokens[i].type == TOK_LBRACE) i++;

            size_t block_start = i, block_end = i;
            int depth = 1;
            while (block_end < token_count && tokens[block_end].type != TOK_EOF) {
                if (tokens[block_end].type == TOK_LBRACE) depth++;
                if (tokens[block_end].type == TOK_RBRACE) {
                    depth--;
                    if (depth == 0) break;
                }
                block_end++;
            }

            Token if_tokens[MAX_TOKENS];
            size_t if_token_count = 0;
            for (size_t j = block_start; j < block_end; j++)
                if_tokens[if_token_count++] = tokens[j];
            if_tokens[if_token_count].type = TOK_EOF;

            convert_tokens_to_cpp(if_tokens, if_token_count, out, indent + 1, is_main);

            i = block_end + 1;

            if (i < token_count && tokens[i].type == TOK_ELSE) {
                i++;
                write_indent(out, indent);
                fprintf(out, "} else {\n");
                if (tokens[i].type == TOK_LBRACE) i++;

                size_t else_start = i, else_end = i;
                depth = 1;
                while (else_end < token_count && tokens[else_end].type != TOK_EOF) {
                    if (tokens[else_end].type == TOK_LBRACE) depth++;
                    if (tokens[else_end].type == TOK_RBRACE) {
                        depth--;
                        if (depth == 0) break;
                    }
                    else_end++;
                }

                Token else_tokens[MAX_TOKENS];
                size_t else_token_count = 0;
                for (size_t j = else_start; j < else_end; j++)
                    else_tokens[else_token_count++] = tokens[j];
                else_tokens[else_token_count].type = TOK_EOF;

                convert_tokens_to_cpp(else_tokens, else_token_count, out, indent + 1, is_main);
                i = else_end + 1;
            }

            write_indent(out, indent);
            fprintf(out, "}\n");
            continue;
        }

        if (tokens[i].type == TOK_LOOP_START) {
            i++;
            size_t expr_start = i;
            while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) {
                i++;
            }
            size_t expr_end = i;
            if (tokens[i].type == TOK_LBRACE) i++;

            int is_simple_ident = (expr_end - expr_start == 1 && tokens[expr_start].type == TOK_IDENT);

            size_t loop_start = i, loop_end = i;
            int depth = 1;
            while (loop_end < token_count && tokens[loop_end].type != TOK_EOF) {
                if (tokens[loop_end].type == TOK_LBRACE) depth++;
                if (tokens[loop_end].type == TOK_RBRACE) {
                    depth--;
                    if (depth == 0) break;
                }
                loop_end++;
            }

            Token loop_tokens[MAX_TOKENS];
            size_t loop_token_count = 0;
            for (size_t j = loop_start; j < loop_end && loop_token_count < MAX_TOKENS - 1; j++)
                loop_tokens[loop_token_count++] = tokens[j];
            loop_tokens[loop_token_count].type = TOK_EOF;

            if (is_main && sdl_window_created) {
                write_indent(out, indent);
                fprintf(out, "{\n");
                write_indent(out, indent + 1);
                fprintf(out, "bool _running = true;\n");
                write_indent(out, indent + 1);
                fprintf(out, "SDL_Event _event;\n");
                write_indent(out, indent + 1);
                fprintf(out, "_last_frame_time = SDL_GetTicks();\n");
                write_indent(out, indent + 1);
                fprintf(out, "while (_running) {\n");
                write_indent(out, indent + 2);
                fprintf(out, "Uint32 _current_time = SDL_GetTicks();\n");
                write_indent(out, indent + 2);
                fprintf(out, "deltaTime = (_current_time - _last_frame_time) / 1000.0;\n");
                write_indent(out, indent + 2);
                fprintf(out, "_last_frame_time = _current_time;\n");
                write_indent(out, indent + 2);
                fprintf(out, "memcpy(_prev_key_state, _key_state, sizeof(_key_state));\n");
                write_indent(out, indent + 2);
                fprintf(out, "while (SDL_PollEvent(&_event)) {\n");
                write_indent(out, indent + 3);
                fprintf(out, "if (_event.type == SDL_QUIT) _running = false;\n");
                write_indent(out, indent + 3);
                fprintf(out, "if (_event.type == SDL_KEYDOWN) _key_state[_event.key.keysym.sym %% 512] = 1;\n");
                write_indent(out, indent + 3);
                fprintf(out, "if (_event.type == SDL_KEYUP) _key_state[_event.key.keysym.sym %% 512] = 0;\n");
                write_indent(out, indent + 2);
                fprintf(out, "}\n");
                convert_tokens_to_cpp(loop_tokens, loop_token_count, out, indent + 2, 0);
                write_indent(out, indent + 2);
                fprintf(out, "SDL_RenderPresent(_renderer);\n");
                write_indent(out, indent + 1);
                fprintf(out, "}\n");
                write_indent(out, indent);
                fprintf(out, "}\n");
            } else if (is_simple_ident) {
                char *counter_name = tokens[expr_start].text;
                write_indent(out, indent);
                fprintf(out, "{ double _rep_%s = %s; for (%s = 0; %s < _rep_%s; %s++) {\n",
                        counter_name, counter_name,
                        counter_name, counter_name, counter_name, counter_name);
                convert_tokens_to_cpp(loop_tokens, loop_token_count, out, indent + 1, is_main);
                write_indent(out, indent);
                fprintf(out, "} }\n");
            } else {
                static int _loop_idx = 0;
                char count_expr[256] = "";
                size_t expr_len = 0;
                for (size_t ei = expr_start; ei < expr_end; ei++) {
                    Token *et = &tokens[ei];
                    const char *part = "";
                    if (et->type == TOK_IDENT || et->type == TOK_NUMBER) part = et->text;
                    else if (et->type == TOK_PLUS)  part = " + ";
                    else if (et->type == TOK_MINUS) part = " - ";
                    else if (et->type == TOK_MUL)   part = " * ";
                    else if (et->type == TOK_DIV)   part = " / ";
                    else if (et->type == TOK_MOD)   part = " % ";
                    else if (et->type == TOK_DOT && ei + 1 < expr_end && tokens[ei+1].type == TOK_IDENT && strcmp(tokens[ei+1].text, "len") == 0) {
                        part = ".size()";
                        ei++;
                    }
                    expr_len += snprintf(count_expr + expr_len, sizeof(count_expr) - expr_len, "%s", part);
                }
                int idx = _loop_idx++;
                write_indent(out, indent);
                fprintf(out, "{ double _rep_%d = %s; for (double _loop_%d = 0; _loop_%d < _rep_%d; _loop_%d++) {\n",
                        idx, count_expr, idx, idx, idx, idx);
                convert_tokens_to_cpp(loop_tokens, loop_token_count, out, indent + 1, is_main);
                write_indent(out, indent);
                fprintf(out, "} }\n");
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
                fprintf(out, "return ");
                i = emit_expression(tokens, i, out);
                fprintf(out, ";\n");
            }
            continue;
        }

        if (tokens[i].type == TOK_END) {
            return;
        }

        fprintf(stderr, "Unknown token in converter at position %zu, type: %d, text: '%s'\n", i, tokens[i].type, tokens[i].text);
        exit(1);
    }
}

void convert_to_cpp(Token tokens[], size_t token_count, const char *output_path) {
    sdl_window_created = 0;
    parse_functions(tokens, token_count);

    FILE *out = fopen(output_path, "w");
    if (!out) { perror("fopen"); exit(1); }

    fprintf(out, "#include <iostream>\n#include <string>\n#include <cstring>\n#include <vector>\n#include <random>\n#include <iomanip>\n#include <cmath>\n");
#if HAVE_SDL
    fprintf(out, "#include <SDL2/SDL.h>\n");
#endif
    fprintf(out, "\n");
    fprintf(out, "struct _KnVal {\n");
    fprintf(out, "    bool _s; double _n; std::string _t;\n");
    fprintf(out, "    _KnVal(double v):_s(false),_n(v){}\n");
    fprintf(out, "    _KnVal(const char* v):_s(true),_n(0),_t(v){}\n");
    fprintf(out, "    _KnVal(const std::string& v):_s(true),_n(0),_t(v){}\n");
    fprintf(out, "    operator double() const { return _n; }\n");
    fprintf(out, "    operator std::string() const { return _t; }\n");
    fprintf(out, "    friend std::ostream& operator<<(std::ostream& os, const _KnVal& v) {\n");
    fprintf(out, "        if (v._s) return os << v._t;\n");
    fprintf(out, "        return os << v._n;\n");
    fprintf(out, "    }\n");
    fprintf(out, "};\n");
    fprintf(out, "using _KnTable = std::vector<_KnVal>;\n\n");
    fprintf(out, "std::mt19937 _rng(std::random_device{}());\n");
    fprintf(out, "double _random(double min, double max) {\n");
    fprintf(out, "    std::uniform_real_distribution<double> dist(min, max);\n");
    fprintf(out, "    return dist(_rng);\n");
    fprintf(out, "}\n");
    fprintf(out, "double _lerp(double a, double b, double t) { return a + (b - a) * t; }\n");
    fprintf(out, "double _distance(double x1, double y1, double x2, double y2) { return std::sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); }\n\n");
#if HAVE_SDL
    fprintf(out, "SDL_Window *_window = nullptr;\n");
    fprintf(out, "SDL_Renderer *_renderer = nullptr;\n");
    fprintf(out, "int _key_state[512] = {0};\n");
    fprintf(out, "int _prev_key_state[512] = {0};\n");
    fprintf(out, "double deltaTime = 0.0;\n");
    fprintf(out, "Uint32 _last_frame_time = 0;\n\n");
    fprintf(out, "bool _key_pressed(const char *key_name) {\n");
    fprintf(out, "    SDL_Keycode kc = SDL_GetKeyFromName(key_name);\n");
    fprintf(out, "    if (kc == SDLK_UNKNOWN) return false;\n");
    fprintf(out, "    int idx = kc %% 512;\n");
    fprintf(out, "    return _key_state[idx] && !_prev_key_state[idx];\n");
    fprintf(out, "}\n\n");
    fprintf(out, "bool _key_down(const char *key_name) {\n");
    fprintf(out, "    SDL_Keycode kc = SDL_GetKeyFromName(key_name);\n");
    fprintf(out, "    if (kc == SDLK_UNKNOWN) return false;\n");
    fprintf(out, "    int idx = kc %% 512;\n");
    fprintf(out, "    return _key_state[idx];\n");
    fprintf(out, "}\n\n");
    fprintf(out, "void _drawCircle(SDL_Renderer *r, int cx, int cy, int radius, int red, int green, int blue) {\n");
    fprintf(out, "    SDL_SetRenderDrawColor(r, red, green, blue, 255);\n");
    fprintf(out, "    int x = radius, y = 0, err = 0;\n");
    fprintf(out, "    while (x >= y) {\n");
    fprintf(out, "        SDL_RenderDrawPoint(r,cx+x,cy+y); SDL_RenderDrawPoint(r,cx+y,cy+x);\n");
    fprintf(out, "        SDL_RenderDrawPoint(r,cx-y,cy+x); SDL_RenderDrawPoint(r,cx-x,cy+y);\n");
    fprintf(out, "        SDL_RenderDrawPoint(r,cx-x,cy-y); SDL_RenderDrawPoint(r,cx-y,cy-x);\n");
    fprintf(out, "        SDL_RenderDrawPoint(r,cx+y,cy-x); SDL_RenderDrawPoint(r,cx+x,cy-y);\n");
    fprintf(out, "        if (err <= 0) { y++; err += 2*y+1; }\n");
    fprintf(out, "        else          { x--; err -= 2*x+1; }\n");
    fprintf(out, "    }\n");
    fprintf(out, "}\n\n");
#endif

    for (size_t fi = 0; fi < function_count; fi++) {
        Function *f = &functions[fi];
        if (strcmp(f->name, "main") == 0) continue;
        RetType rt = detect_return_type(f->tokens, f->token_count);
        const char *ret = rt == RT_STRING ? "std::string" : rt == RT_DOUBLE ? "double" : "void";
        fprintf(out, "%s %s(", ret, f->name);
        for (size_t pi = 0; pi < f->param_count; pi++) {
            if (pi > 0) fprintf(out, ", ");
            if (param_is_array(f->param_names[pi], f->tokens, f->token_count)) {
                fprintf(out, "_KnTable& %s", f->param_names[pi]);
            } else {
                fprintf(out, "double %s", f->param_names[pi]);
            }
        }
        fprintf(out, ");\n");
    }
    if (function_count > 1) fprintf(out, "\n");

    for (size_t fi = 0; fi < function_count; fi++) {
        Function *f = &functions[fi];
        if (strcmp(f->name, "main") == 0) continue;
        RetType rt = detect_return_type(f->tokens, f->token_count);
        const char *ret = rt == RT_STRING ? "std::string" : rt == RT_DOUBLE ? "double" : "void";
        fprintf(out, "%s %s(", ret, f->name);
        for (size_t pi = 0; pi < f->param_count; pi++) {
            if (pi > 0) fprintf(out, ", ");
            if (param_is_array(f->param_names[pi], f->tokens, f->token_count)) {
                fprintf(out, "_KnTable& %s", f->param_names[pi]);
            } else {
                fprintf(out, "double %s", f->param_names[pi]);
            }
        }
        fprintf(out, ") {\n");
        convert_tokens_to_cpp(f->tokens, f->token_count, out, 1, 0);
        fprintf(out, "}\n\n");
    }

    Function *main_func = get_function("main");
    if (main_func) {
        fprintf(out, "int main() {\n");
        convert_tokens_to_cpp(main_func->tokens, main_func->token_count, out, 1, 1);
#if HAVE_SDL
        if (sdl_window_created) {
            fprintf(out, "    SDL_DestroyRenderer(_renderer);\n");
            fprintf(out, "    SDL_DestroyWindow(_window);\n");
            fprintf(out, "    SDL_Quit();\n");
        }
#endif
        fprintf(out, "    return 0;\n}\n");
    }

    fclose(out);
}

int has_extension(const char *name, const char *ext) {
    size_t nlen = strlen(name);
    size_t elen = strlen(ext);
    if (nlen < elen) return 0;
    return strcmp(name + nlen - elen, ext) == 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s file.kn\n", argv[0]);
        fprintf(stderr, "  ^ compile and run (generates .cpp and executable)\n");
        return 1;
    }

    Token tokens[MAX_TOKENS];
    size_t token_count;
    int compile_and_run = argc == 2;

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0) {
        fprintf(stderr, "The file is empty\n");
        fclose(f);
        return 1;
    }

    char *source = malloc(size + 1);
    if (!source) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    token_count = tokenize(source, tokens);

    Token expanded_tokens[MAX_EXPANDED_TOKENS];
    memset(expanded_tokens, 0, sizeof(expanded_tokens));
    Token *final_tokens = tokens;
    size_t final_count = token_count;

    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_ADD) {
            final_count = process_includes(tokens, token_count, expanded_tokens, MAX_EXPANDED_TOKENS);
            final_tokens = expanded_tokens;
            break;
        }
    }

#if HAVE_SDL == 0
    fprintf(stderr, "Warning: SDL2 not found. Install it with: sudo apt install libsdl2-dev\n");
#endif

    if (compile_and_run) {
        char output_path[256];
        strcpy(output_path, argv[1]);
        size_t len = strlen(output_path);
        if (len > 3 && strcmp(output_path + len - 3, ".kn") == 0) {
            strcpy(output_path + len - 3, ".cpp");
        } else {
            strcat(output_path, ".cpp");
        }
        convert_to_cpp(final_tokens, final_count, output_path);

        char basename[256];
        strcpy(basename, output_path);
        len = strlen(basename);
        if (len > 4 && strcmp(basename + len - 4, ".cpp") == 0) {
            basename[len - 4] = 0;
        }

        char command[512];
        snprintf(command, sizeof(command),
            "g++ -std=c++17 %s -o %s $(pkg-config --cflags --libs sdl2 2>/dev/null) && ./%s",
            output_path, basename, basename);
        fprintf(stderr, "Compiling and running...\n");
        system(command);
    } else {
        fprintf(stderr, "Error: kinnie can only compile .kn files to C++.\n");
        fprintf(stderr, "Usage: %s file.kn\n", argv[0]);
    }

    free(source);
    return 0;
}
