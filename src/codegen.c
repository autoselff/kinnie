#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "codegen.h"
#include "parser.h"

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000.0) + (ts.tv_nsec / 1000000.0);
}

typedef enum { RT_VOID, RT_DOUBLE, RT_STRING } RetType;

static int sdl_window_created = 0;

static void write_indent(FILE *out, int indent) {
    for (int k = 0; k < indent; k++) fputs("    ", out);
}

// Maps Kinnie built-in function names to their C++ equivalents.
static const char *map_builtin(const char *name) {
    if (!strcmp(name, "sin"))       return "std::sin";
    if (!strcmp(name, "cos"))       return "std::cos";
    if (!strcmp(name, "abs"))       return "std::fabs";
    if (!strcmp(name, "exp"))       return "std::exp";
    if (!strcmp(name, "log"))       return "std::log";
    if (!strcmp(name, "log10"))     return "std::log10";
    if (!strcmp(name, "pow"))       return "std::pow";
    if (!strcmp(name, "sqrt"))      return "std::sqrt";
    if (!strcmp(name, "min"))       return "std::min<double>";
    if (!strcmp(name, "max"))       return "std::max<double>";
    if (!strcmp(name, "round"))     return "std::round";
    if (!strcmp(name, "floor"))     return "std::floor";
    if (!strcmp(name, "ceil"))      return "std::ceil";
    if (!strcmp(name, "lerp"))      return "_lerp";
    if (!strcmp(name, "distance"))  return "_distance";
    if (!strcmp(name, "clamp"))     return "_clamp";
    if (!strcmp(name, "mod"))       return "std::fmod";
    if (!strcmp(name, "random"))    return "_random";
    if (!strcmp(name, "sizeof"))    return "sizeof";
    if (!strcmp(name, "delay"))     return "_delay";
    return NULL;
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

static int is_builtin_call(Token tokens[], size_t i, const char *name) {
    return tokens[i].type == TOK_IDENT
        && tokens[i + 1].type == TOK_LBRACKET
        && strcmp(tokens[i].text, name) == 0;
}

static void emit_string_expr(const char *str, FILE *out) {
    int has_interp = 0;
    for (size_t si = 0; str[si]; si++) {
        if (str[si] == '{') { has_interp = 1; break; }
    }
    if (!has_interp) {
        fputc('"', out);
        for (size_t si = 0; str[si]; si++) {
            if (str[si] == '"') fputs("\\\"", out);
            else fputc(str[si], out);
        }
        fputc('"', out);
        return;
    }
    fputs("(std::ostringstream()", out);
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
            fprintf(out, " << %s", vname);
        } else {
            fputs(" << \"", out);
            while (str[j] != '\0' && str[j] != '{') {
                if (str[j] == '"') fputs("\\\"", out);
                else fputc(str[j], out);
                j++;
            }
            fputc('"', out);
        }
    }
    fputs(").str()", out);
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

    if (tokens[i].type == TOK_STRING) {
        fputs("_KnVal(", out);
        emit_string_expr(tokens[i].text, out);
        fputc(')', out);
        return i + 1;
    }

    if (tokens[i].type == TOK_IDENT) {
        if (tokens[i + 1].type == TOK_LBRACKET && strcmp(tokens[i].text, "len") == 0) {
            i += 2;
            if (tokens[i + 1].type == TOK_LSQUARE) {
                fprintf(out, "%s[(int)(", tokens[i].text);
                i += 2;
                i = emit_expression(tokens, i, out);
                if (tokens[i].type == TOK_RSQUARE) i++;
                fputs(")]", out);
                while (tokens[i].type == TOK_LSQUARE) {
                    fputs("._a[(int)(", out);
                    i++;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_RSQUARE) i++;
                    fputs(")]", out);
                }
                fputs(".size()", out);
            } else {
                fprintf(out, "%s.size()", tokens[i].text);
                i++;
            }
            if (tokens[i].type == TOK_RBRACKET) i++;
            return i;
        }
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
            while (tokens[i].type == TOK_LSQUARE) {
                fputs("._a[(int)(", out);
                i++;
                i = emit_expression(tokens, i, out);
                if (tokens[i].type == TOK_RSQUARE) i++;
                fputs(")]", out);
            }
            return i;
        }
        if (tokens[i + 1].type == TOK_DOT && tokens[i + 2].type == TOK_IDENT) {
            if (tokens[i + 3].type == TOK_LBRACKET) {
                fprintf(out, "%s.%s(", tokens[i].text, tokens[i + 2].text);
                i += 4;
                int first_d = 1;
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (!first_d) fputs(", ", out);
                    first_d = 0;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                fputc(')', out);
                if (tokens[i].type == TOK_RBRACKET) i++;
                return i;
            }
            fprintf(out, "%s.%s", tokens[i].text, tokens[i + 2].text);
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

    if (tokens[i].type == TOK_LSQUARE) {
        fputs("_KnVal(_KnTable{", out);
        i++;
        int _first = 1;
        while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
            if (!_first) fputs(", ", out);
            _first = 0;
            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "\"%s\"", tokens[i].text);
                i++;
            } else {
                i = emit_expression(tokens, i, out);
            }
            if (tokens[i].type == TOK_COMMA) i++;
        }
        if (tokens[i].type == TOK_RSQUARE) i++;
        fputs("})", out);
        return i;
    }

    fprintf(stderr, "Expected value in expression: got type %d text '%s'\n",
            tokens[i].type, tokens[i].text);
    exit(1);
}

static size_t emit_expression(Token tokens[], size_t i, FILE *out) {
    i = emit_value(tokens, i, out);
    while (tokens[i].type == TOK_PLUS  || tokens[i].type == TOK_MINUS
        || tokens[i].type == TOK_MUL   || tokens[i].type == TOK_DIV
        || tokens[i].type == TOK_MOD) {
        const char *op =
            tokens[i].type == TOK_PLUS  ? " + " :
            tokens[i].type == TOK_MINUS ? " - " :
            tokens[i].type == TOK_MUL   ? " * " :
            tokens[i].type == TOK_DIV   ? " / " : " % ";
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
            const char *str = tokens[i].text;
            int has_interp = 0;
            for (size_t si = 0; str[si]; si++) {
                if (str[si] == '{') { has_interp = 1; break; }
            }

            if (has_interp) {
                bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "(std::ostringstream()");
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
                        bi += snprintf(buf + bi, ARG_BUF_LEN - bi, " << %s", vname);
                    } else {
                        bi += snprintf(buf + bi, ARG_BUF_LEN - bi, " << \"");
                        while (str[j] != '\0' && str[j] != '{') {
                            if (str[j] == '"') bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "\\\"");
                            else bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "%c", str[j]);
                            j++;
                        }
                        bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "\"");
                    }
                }
                bi += snprintf(buf + bi, ARG_BUF_LEN - bi, ").str().c_str()");
            } else {
                bi += snprintf(buf + bi, ARG_BUF_LEN - bi, "\"%s\"", str);
            }
            i++;
        } else {
            while (tokens[i].type != TOK_COMMA && tokens[i].type != TOK_RBRACKET
                   && tokens[i].type != TOK_EOF) {
                const char *part = NULL;
                switch (tokens[i].type) {
                    case TOK_NUMBER:
                    case TOK_IDENT:  part = tokens[i].text; break;
                    case TOK_PLUS:   part = " + "; break;
                    case TOK_MINUS:  part = " - "; break;
                    case TOK_MUL:    part = " * "; break;
                    case TOK_DIV:    part = " / "; break;
                    case TOK_MOD:    part = " % "; break;
                    case TOK_DOT:    part = ".";   break;
                    case TOK_LSQUARE: part = "[(int)("; break;
                    case TOK_RSQUARE: part = ")]";  break;
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
        } else {
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
        if (tokens[i].type == TOK_IDENT
            && strcmp(tokens[i].text, param_name) == 0
            && i + 1 < token_count
            && tokens[i + 1].type == TOK_LSQUARE)
            return 1;
        if (tokens[i].type == TOK_STRING) {
            char search[MAX_STRING_LEN + 4];
            snprintf(search, sizeof(search), "%s[", param_name);
            if (strstr(tokens[i].text, search)) return 1;
        }
    }
    return 0;
}

static int param_is_2d_array(const char *param_name, Token tokens[], size_t token_count) {
    (void)param_name;
    (void)tokens;
    (void)token_count;
    return 0;
}

static const char *param_struct_type(const char *param_name, Token tokens[], size_t token_count) {
    for (size_t i = 0; i + 2 < token_count; i++) {
        if (tokens[i].type == TOK_IDENT
                && strcmp(tokens[i].text, param_name) == 0
                && tokens[i + 1].type == TOK_DOT
                && tokens[i + 2].type == TOK_IDENT) {
            const char *member = tokens[i + 2].text;
            for (size_t ci = 0; ci < struct_count; ci++) {
                for (size_t mi = 0; mi < structs[ci].method_count; mi++)
                    if (strcmp(structs[ci].methods[mi].name, member) == 0)
                        return structs[ci].name;
                for (size_t fi = 0; fi < structs[ci].field_count; fi++)
                    if (strcmp(structs[ci].field_names[fi], member) == 0)
                        return structs[ci].name;
            }
        }
    }
    return NULL;
}

static void emit_func_signature(FILE *out, Function *f) {
    RetType rt = detect_return_type(f->tokens, f->token_count);
    const char *ret = rt == RT_STRING ? "std::string" : rt == RT_DOUBLE ? "double" : "void";
    fprintf(out, "%s %s(", ret, f->name);
    for (size_t pi = 0; pi < f->param_count; pi++) {
        if (pi > 0) fputs(", ", out);
        if (param_is_2d_array(f->param_names[pi], f->tokens, f->token_count)) {
            fprintf(out, "_KnTable2D& %s", f->param_names[pi]);
        } else if (param_is_array(f->param_names[pi], f->tokens, f->token_count)) {
            fprintf(out, "_KnTable& %s", f->param_names[pi]);
        } else {
            const char *ct = param_struct_type(f->param_names[pi], f->tokens, f->token_count);
            if (ct) fprintf(out, "%s& %s", ct, f->param_names[pi]);
            else    fprintf(out, "_KnVal %s", f->param_names[pi]);
        }
    }
    fputc(')', out);
}

static int is_comp_op(TokenType t) {
    return t == TOK_EQUALS || t == TOK_NOT_EQUALS || t == TOK_MORE ||
           t == TOK_LESS   || t == TOK_MORE_EQUALS || t == TOK_LESS_EQUALS;
}

static size_t emit_simple_cond(Token tokens[], size_t i, FILE *out) {
    if (tokens[i].type == TOK_NOT) {
        fputs("!(", out);
        i = emit_simple_cond(tokens, i + 1, out);
        fputc(')', out);
        return i;
    }
    if (tokens[i].type == TOK_KEY_PRESSED || tokens[i].type == TOK_KEY_DOWN) {
        const char *fn = (tokens[i].type == TOK_KEY_PRESSED) ? "_key_pressed" : "_key_down";
        i++;
        if (tokens[i].type == TOK_LBRACKET) i++;
        if (tokens[i].type == TOK_STRING) { fprintf(out, "%s(\"%s\")", fn, tokens[i].text); i++; }
        if (tokens[i].type == TOK_RBRACKET) i++;
        return i;
    }
    if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET
            && !map_builtin(tokens[i].text) && strcmp(tokens[i].text, "len") != 0) {
        fprintf(out, "%s(", tokens[i].text);
        i += 2;
        int first = 1;
        while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
            if (!first) fputs(", ", out);
            first = 0;
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_COMMA) i++;
        }
        fputc(')', out);
        if (tokens[i].type == TOK_RBRACKET) i++;
    } else {
        i = emit_expression(tokens, i, out);
    }
    if (is_comp_op(tokens[i].type)) {
        fprintf(out, " %s ", comp_op_str(tokens[i].type));
        i++;
        i = emit_expression(tokens, i, out);
    } else {
        fputs(" != 0", out);
    }
    return i;
}

static size_t emit_condition(Token tokens[], size_t i, FILE *out) {
    i = emit_simple_cond(tokens, i, out);
    while (tokens[i].type == TOK_AND || tokens[i].type == TOK_OR) {
        fputs(tokens[i].type == TOK_AND ? " && " : " || ", out);
        i++;
        i = emit_simple_cond(tokens, i, out);
    }
    return i;
}

void convert_tokens_to_cpp(Token tokens[], size_t token_count, FILE *out, int indent, int is_main) {
    size_t i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {

        if (tokens[i].type == TOK_STRUCT) {
            i++;
            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_IDENT) {
                write_indent(out, indent);
                fprintf(out, "%s %s;\n", tokens[i].text, tokens[i + 1].text);
                i += 2;
            }
            continue;
        }

        if (is_builtin_call(tokens, i, "createWindow")) {
            i += 2;
            char b[3][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 3);
            write_indent(out, indent);
            fputs("SDL_Init(SDL_INIT_VIDEO);\n", out);
            write_indent(out, indent);
            fprintf(out,
                "_window = SDL_CreateWindow(%s, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, %s, %s, SDL_WINDOW_SHOWN);\n",
                b[2], b[0], b[1]);
            write_indent(out, indent);
            fputs("_renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);\n", out);
            write_indent(out, indent);
            fputs("#ifdef SDL_TTF_H_\n", out);
            write_indent(out, indent);
            fputs("TTF_Init();\n", out);
            write_indent(out, indent);
            fputs("#endif\n", out);
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
            fprintf(out, "  SDL_Rect _sq = {(int)(%s),(int)(%s),(int)(%s),(int)(%s)};\n",
                    b[0], b[1], b[2], b[2]);
            write_indent(out, indent);
            fputs("  SDL_RenderFillRect(_renderer, &_sq); }\n", out);
            continue;
        }

        if (is_builtin_call(tokens, i, "drawRectangle")) {
            i += 2;
            char b[7][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 7);
            write_indent(out, indent);
            fprintf(out, "{ SDL_SetRenderDrawColor(_renderer, %s, %s, %s, 255);\n", b[4], b[5], b[6]);
            write_indent(out, indent);
            fprintf(out, "  SDL_Rect _rect = {(int)(%s),(int)(%s),(int)(%s),(int)(%s)};\n",
                    b[0], b[1], b[2], b[3]);
            write_indent(out, indent);
            fputs("  SDL_RenderFillRect(_renderer, &_rect); }\n", out);
            continue;
        }

        if (is_builtin_call(tokens, i, "drawCircle")) {
            i += 2;
            char b[6][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 6);
            write_indent(out, indent);
            fprintf(out,
                "_drawCircle(_renderer, (int)(%s),(int)(%s),(int)(%s),(int)(%s),(int)(%s),(int)(%s));\n",
                b[0], b[1], b[2], b[3], b[4], b[5]);
            continue;
        }

        if (is_builtin_call(tokens, i, "drawText")) {
            i += 2;
            char b[7][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 7);
            write_indent(out, indent);
            fprintf(out,
                "_drawText(_renderer, (int)(%s),(int)(%s),%s,(int)(%s),%s,%s,%s);\n",
                b[0], b[1], b[2], b[3], b[4], b[5], b[6]);
            continue;
        }

        if (is_builtin_call(tokens, i, "setFont")) {
            i += 2;
            char b[1][ARG_BUF_LEN];
            i = parse_call_args(tokens, i, b, 1);
            write_indent(out, indent);
            fprintf(out, "_font_path = %s;\n", b[0]);
            continue;
        }

        if (tokens[i].type == TOK_STOP) {
            i++;
            write_indent(out, indent);
            fputs("break;\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_IDENT) {
            int is_struct = 0;
            for (size_t si = 0; si < struct_count; si++) {
                if (strcmp(tokens[i].text, structs[si].name) == 0) {
                    is_struct = 1;
                    break;
                }
            }
            if (is_struct) {
                write_indent(out, indent);
                fprintf(out, "%s %s;\n", tokens[i].text, tokens[i + 1].text);
                i += 2;
                continue;
            }
        }

        if (tokens[i].type == TOK_VAR) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i + 1].text);
            i += 3;
            write_indent(out, indent);

            if (tokens[i].type == TOK_LSQUARE) {
                i++;
                if (tokens[i].type == TOK_LSQUARE) {
                    fprintf(out, "_KnTable %s;\n", name);
                    write_indent(out, indent);
                    fprintf(out, "{ _KnTable _tmp_%s;\n", name);
                    while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                        if (tokens[i].type == TOK_LSQUARE) {
                            write_indent(out, indent + 1);
                            fputs("{ _KnTable _row;\n", out);
                            i++;
                            while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                                if (tokens[i].type == TOK_LSQUARE) {
                                    write_indent(out, indent + 2);
                                    fputs("{ _KnTable _inner;\n", out);
                                    i++;
                                    while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                                        write_indent(out, indent + 2);
                                        fputs("_inner.push_back(_KnVal(", out);
                                        if (tokens[i].type == TOK_STRING) {
                                            fprintf(out, "\"%s\"", tokens[i].text);
                                            i++;
                                        } else {
                                            i = emit_expression(tokens, i, out);
                                        }
                                        fputs("));\n", out);
                                        if (tokens[i].type == TOK_COMMA) i++;
                                    }
                                    if (tokens[i].type == TOK_RSQUARE) i++;
                                    write_indent(out, indent + 2);
                                    fputs("_row.push_back(_KnVal(_inner));\n", out);
                                    write_indent(out, indent + 2);
                                    fputs("}\n", out);
                                } else {
                                    write_indent(out, indent + 2);
                                    fputs("_row.push_back(_KnVal(", out);
                                    if (tokens[i].type == TOK_STRING) {
                                        fprintf(out, "\"%s\"", tokens[i].text);
                                        i++;
                                    } else {
                                        i = emit_expression(tokens, i, out);
                                    }
                                    fputs("));\n", out);
                                }
                                if (tokens[i].type == TOK_COMMA) i++;
                            }
                            if (tokens[i].type == TOK_RSQUARE) i++;
                            write_indent(out, indent + 1);
                            fprintf(out, "_tmp_%s.push_back(_KnVal(_row));\n", name);
                            write_indent(out, indent + 1);
                            fputs("}\n", out);
                        } else {
                            i++;
                        }
                        if (tokens[i].type == TOK_COMMA) i++;
                    }
                    if (tokens[i].type == TOK_RSQUARE) i++;
                    write_indent(out, indent + 1);
                    fprintf(out, "%s = _tmp_%s;\n", name, name);
                    write_indent(out, indent);
                    fputs("}\n", out);
                    continue;
                }
                fprintf(out, "_KnTable %s;\n", name);
                write_indent(out, indent);
                fprintf(out, "{ _KnTable _tmp_%s;\n", name);
                while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                    write_indent(out, indent + 1);
                    fprintf(out, "_tmp_%s.push_back(_KnVal(", name);
                    if (tokens[i].type == TOK_STRING) {
                        fprintf(out, "\"%s\"", tokens[i].text);
                        i++;
                    } else {
                        i = emit_expression(tokens, i, out);
                    }
                    fputs("));\n", out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                if (tokens[i].type == TOK_RSQUARE) i++;
                write_indent(out, indent + 1);
                fprintf(out, "%s = _tmp_%s;\n", name, name);
                write_indent(out, indent);
                fputs("}\n", out);
                continue;
            }

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET
                    && strcmp(tokens[i].text, "len") == 0) {
                i += 2;
                if (tokens[i + 1].type == TOK_LSQUARE) {
                    char arrname[MAX_NAME_LEN];
                    strcpy(arrname, tokens[i].text);
                    i += 2;
                    fprintf(out, "double %s = %s[(int)(", name, arrname);
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_RSQUARE) i++;
                    fputs(")]", out);
                    while (tokens[i].type == TOK_LSQUARE) {
                        fputs("._a[(int)(", out);
                        i++;
                        i = emit_expression(tokens, i, out);
                        if (tokens[i].type == TOK_RSQUARE) i++;
                        fputs(")]", out);
                    }
                    fputs(".size();\n", out);
                } else {
                    fprintf(out, "double %s = %s.size()", name, tokens[i].text);
                    i++;
                    fputs(";\n", out);
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
                continue;
            }

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET
                    && !map_builtin(tokens[i].text)) {
                const char *fn = tokens[i].text;
                i += 2;
                fprintf(out, "auto %s = %s(", name, fn);
                i = emit_call_args_inline(tokens, i, out);
                fputs(";\n", out);
                continue;
            }

            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "std::string %s = ", name);
                emit_string_expr(tokens[i].text, out);
                fputs(";\n", out);
                i++;
                continue;
            }

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_DOT) {
                fprintf(out, "auto %s = ", name);
                i = emit_expression(tokens, i, out);
                fputs(";\n", out);
                continue;
            }

            fprintf(out, "double %s = ", name);
            i = emit_expression(tokens, i, out);
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LSQUARE) {
            size_t _scan = i + 2;
            int _sdepth = 1;
            while (_scan < token_count && tokens[_scan].type != TOK_EOF) {
                if (tokens[_scan].type == TOK_LSQUARE) _sdepth++;
                else if (tokens[_scan].type == TOK_RSQUARE) { _sdepth--; if (_sdepth == 0) break; }
                _scan++;
            }
            if (tokens[_scan].type == TOK_RSQUARE
                && tokens[_scan+1].type == TOK_DOT
                && (tokens[_scan+2].type == TOK_IDENT || tokens[_scan+2].type == TOK_ADD)
                && tokens[_scan+3].type == TOK_LBRACKET
                && (strcmp(tokens[_scan+2].text, "remove") == 0
                    || strcmp(tokens[_scan+2].text, "add") == 0)) {
                const char *_arrname = tokens[i].text;
                const char *_method  = tokens[_scan+2].text;
                i += 2;
                write_indent(out, indent);
                fprintf(out, "{ auto& _kn_ref = %s[(int)(", _arrname);
                i = emit_expression(tokens, i, out);
                if (tokens[i].type == TOK_RSQUARE) i++;
                fputs(")]._a;\n", out);
                i++;
                i++;
                i++;
                write_indent(out, indent + 1);
                if (strcmp(_method, "remove") == 0) {
                    fputs("_kn_ref.erase(_kn_ref.begin() + (int)(", out);
                    i = emit_expression(tokens, i, out);
                    fputs("));\n", out);
                } else {
                    if (tokens[i].type == TOK_LSQUARE) {
                        fputs("{ _KnTable _tmp_add;\n", out);
                        i++;
                        while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                            write_indent(out, indent + 2);
                            fputs("_tmp_add.push_back(_KnVal(", out);
                            if (tokens[i].type == TOK_STRING) { fprintf(out, "\"%s\"", tokens[i].text); i++; }
                            else i = emit_expression(tokens, i, out);
                            fputs("));\n", out);
                            if (tokens[i].type == TOK_COMMA) i++;
                        }
                        if (tokens[i].type == TOK_RSQUARE) i++;
                        write_indent(out, indent + 1);
                        fputs("_kn_ref.push_back(_KnVal(_tmp_add)); }\n", out);
                    } else {
                        fputs("_kn_ref.push_back(_KnVal(", out);
                        if (tokens[i].type == TOK_STRING) { fprintf(out, "\"%s\"", tokens[i].text); i++; }
                        else i = emit_expression(tokens, i, out);
                        fputs("));\n", out);
                    }
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
                write_indent(out, indent);
                fputs("}\n", out);
                continue;
            }

            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i].text);
            i += 2;
            write_indent(out, indent);
            fprintf(out, "%s[(int)(", name);
            i = emit_expression(tokens, i, out);
            if (tokens[i].type == TOK_RSQUARE) i++;
            while (tokens[i].type == TOK_LSQUARE) {
                fputs(")]._a[(int)(", out);
                i++;
                i = emit_expression(tokens, i, out);
                if (tokens[i].type == TOK_RSQUARE) i++;
            }
            if (tokens[i].type == TOK_ASSIGN) i++;
            fputs(")] = _KnVal(", out);
            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "\"%s\"", tokens[i].text);
                i++;
            } else {
                i = emit_expression(tokens, i, out);
            }
            fputs(");\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_DOT) {
            size_t lookahead = i;
            char full_chain[MAX_STRING_LEN] = "";
            int chain_count = 0;

            while (lookahead < token_count && tokens[lookahead].type == TOK_IDENT) {
                if (chain_count == 0) {
                    snprintf(full_chain, MAX_STRING_LEN, "%s", tokens[lookahead].text);
                } else {
                    size_t cur_len = strlen(full_chain);
                    snprintf(full_chain + cur_len, MAX_STRING_LEN - cur_len, ".%s", tokens[lookahead].text);
                }
                chain_count++;
                lookahead++;

                if (lookahead < token_count && tokens[lookahead].type == TOK_DOT) {
                    lookahead++;
                    if (lookahead >= token_count || tokens[lookahead].type != TOK_IDENT) {
                        break;
                    }
                } else {
                    break;
                }
            }

            if (lookahead < token_count && tokens[lookahead].type == TOK_LBRACKET && chain_count > 1) {
                write_indent(out, indent);
                fprintf(out, "%s(", full_chain);
                lookahead++;
                int first_arg = 1;
                while (lookahead < token_count && tokens[lookahead].type != TOK_RBRACKET && tokens[lookahead].type != TOK_EOF) {
                    if (!first_arg) fputs(", ", out);
                    first_arg = 0;
                    lookahead = emit_expression(tokens, lookahead, out);
                    if (lookahead < token_count && tokens[lookahead].type == TOK_COMMA) lookahead++;
                }
                if (lookahead < token_count && tokens[lookahead].type == TOK_RBRACKET) lookahead++;
                fputs(");\n", out);
                i = lookahead;
                continue;
            }
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_DOT
                && (tokens[i + 2].type == TOK_IDENT || tokens[i + 2].type == TOK_ADD)) {
            if (tokens[i + 3].type == TOK_LBRACKET) {
                const char *_obj    = tokens[i].text;
                const char *_method = tokens[i + 2].text;
                i += 4;

                if (strcmp(_method, "add") == 0) {
                    write_indent(out, indent);
                    if (tokens[i].type == TOK_LSQUARE) {
                        fputs("{ _KnTable _tmp_add;\n", out);
                        i++;
                        while (tokens[i].type != TOK_RSQUARE && tokens[i].type != TOK_EOF) {
                            write_indent(out, indent + 1);
                            fputs("_tmp_add.push_back(_KnVal(", out);
                            if (tokens[i].type == TOK_STRING) { fprintf(out, "\"%s\"", tokens[i].text); i++; }
                            else i = emit_expression(tokens, i, out);
                            fputs("));\n", out);
                            if (tokens[i].type == TOK_COMMA) i++;
                        }
                        if (tokens[i].type == TOK_RSQUARE) i++;
                        write_indent(out, indent);
                        fprintf(out, "%s.push_back(_KnVal(_tmp_add)); }\n", _obj);
                    } else {
                        fprintf(out, "%s.push_back(_KnVal(", _obj);
                        if (tokens[i].type == TOK_STRING) { fprintf(out, "\"%s\"", tokens[i].text); i++; }
                        else i = emit_expression(tokens, i, out);
                        fputs("));\n", out);
                    }
                    if (tokens[i].type == TOK_RBRACKET) i++;
                    continue;
                }

                if (strcmp(_method, "remove") == 0) {
                    write_indent(out, indent);
                    fprintf(out, "%s.erase(%s.begin() + (int)(", _obj, _obj);
                    i = emit_expression(tokens, i, out);
                    fputs("));\n", out);
                    if (tokens[i].type == TOK_RBRACKET) i++;
                    continue;
                }

                write_indent(out, indent);
                fprintf(out, "%s.%s(", _obj, _method);
                int first_arg = 1;
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    if (!first_arg) fputs(", ", out);
                    first_arg = 0;
                    i = emit_expression(tokens, i, out);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                if (tokens[i].type == TOK_RBRACKET) i++;
                fputs(");\n", out);
                continue;
            }
            if (tokens[i + 3].type == TOK_ASSIGN) {
                write_indent(out, indent);
                fprintf(out, "%s.%s = ", tokens[i].text, tokens[i + 2].text);
                i += 4;
                if (tokens[i].type == TOK_STRING) {
                    fprintf(out, "\"%s\";\n", tokens[i].text);
                    i++;
                } else {
                    i = emit_expression(tokens, i, out);
                    fputs(";\n", out);
                }
                continue;
            }
        }

        if (tokens[i].type == TOK_IDENT
                && (tokens[i + 1].type == TOK_INCREMENT || tokens[i + 1].type == TOK_DECREMENT)) {
            write_indent(out, indent);
            fprintf(out, "%s%s;\n", tokens[i].text,
                    tokens[i + 1].type == TOK_INCREMENT ? "++" : "--");
            i += 2;
            continue;
        }

        if (tokens[i].type == TOK_IDENT
                && (tokens[i + 1].type == TOK_PLUS_ASSIGN  || tokens[i + 1].type == TOK_MINUS_ASSIGN
                 || tokens[i + 1].type == TOK_MUL_ASSIGN   || tokens[i + 1].type == TOK_DIV_ASSIGN)) {
            const char *op =
                tokens[i + 1].type == TOK_PLUS_ASSIGN  ? "+=" :
                tokens[i + 1].type == TOK_MINUS_ASSIGN ? "-=" :
                tokens[i + 1].type == TOK_MUL_ASSIGN   ? "*=" : "/=";
            write_indent(out, indent);
            fprintf(out, "%s %s ", tokens[i].text, op);
            i += 2;
            i = emit_expression(tokens, i, out);
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_ASSIGN) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i].text);
            i += 2;
            write_indent(out, indent);

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET
                    && !map_builtin(tokens[i].text)) {
                const char *fn = tokens[i].text;
                i += 2;
                fprintf(out, "%s = %s(", name, fn);
                i = emit_call_args_inline(tokens, i, out);
                fputs(";\n", out);
                continue;
            }

            if (tokens[i].type == TOK_STRING) {
                fprintf(out, "%s = ", name);
                emit_string_expr(tokens[i].text, out);
                fputs(";\n", out);
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

        if (tokens[i].type == TOK_PRINT) {
            i++;
            write_indent(out, indent);
            fputs("std::cout << ", out);
            if (tokens[i].type == TOK_STRING) {
                emit_string_stream(tokens[i].text, out);
                i++;
            } else {
                i = emit_expression(tokens, i, out);
            }
            fputs(";\n", out);
            continue;
        }

        if (tokens[i].type == TOK_IF_START) {
            i++;
            write_indent(out, indent);
            fputs("if (", out);
            i = emit_condition(tokens, i, out);
            fputs(") {\n", out);

            while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) i++;
            if (tokens[i].type == TOK_LBRACE) i++;

            size_t end = find_block_end(tokens, token_count, i);
            Token block[MAX_TOKENS];
            size_t bc = copy_block(tokens, i, end, block);
            convert_tokens_to_cpp(block, bc, out, indent + 1, is_main);
            i = end + 1;

            while (i < token_count && tokens[i].type == TOK_ELSE) {
                i++;
                if (tokens[i].type == TOK_IF_START) {
                    i++;
                    write_indent(out, indent);
                    fputs("} else if (", out);
                    i = emit_condition(tokens, i, out);
                    fputs(") {\n", out);
                    while (i < token_count && tokens[i].type != TOK_LBRACE && tokens[i].type != TOK_EOF) i++;
                    if (tokens[i].type == TOK_LBRACE) i++;
                    size_t elif_end = find_block_end(tokens, token_count, i);
                    bc = copy_block(tokens, i, elif_end, block);
                    convert_tokens_to_cpp(block, bc, out, indent + 1, is_main);
                    i = elif_end + 1;
                } else {
                    write_indent(out, indent);
                    fputs("} else {\n", out);
                    if (tokens[i].type == TOK_LBRACE) i++;
                    size_t e_end = find_block_end(tokens, token_count, i);
                    bc = copy_block(tokens, i, e_end, block);
                    convert_tokens_to_cpp(block, bc, out, indent + 1, is_main);
                    i = e_end + 1;
                    break;
                }
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

            int is_simple_ident = (expr_end - expr_start == 1
                                   && tokens[expr_start].type == TOK_IDENT);

            int is_while_cond = 0;
            for (size_t ci = expr_start; ci < expr_end; ci++) {
                TokenType tt = tokens[ci].type;
                if (is_comp_op(tt) || tt == TOK_AND || tt == TOK_OR || tt == TOK_NOT) {
                    is_while_cond = 1;
                    break;
                }
            }

            size_t loop_end = find_block_end(tokens, token_count, i);
            Token loop_tokens[MAX_TOKENS];
            size_t lc = copy_block(tokens, i, loop_end, loop_tokens);

            if (is_main && sdl_window_created) {
                write_indent(out, indent);     fputs("{\n", out);
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
                write_indent(out, indent);     fputs("}\n", out);
            } else if (is_while_cond) {
                write_indent(out, indent);
                fputs("while (", out);
                emit_condition(tokens, expr_start, out);
                fputs(") {\n", out);
                convert_tokens_to_cpp(loop_tokens, lc, out, indent + 1, is_main);
                write_indent(out, indent);
                fputs("}\n", out);
            } else if (is_simple_ident) {
                const char *c = tokens[expr_start].text;
                write_indent(out, indent);
                fprintf(out, "{ double _rep_%s = %s; for (%s = 0; %s < _rep_%s; %s++) {\n",
                        c, c, c, c, c, c);
                convert_tokens_to_cpp(loop_tokens, lc, out, indent + 1, is_main);
                write_indent(out, indent);
                fputs("} }\n", out);
            } else {
                static int loop_idx = 0;
                char expr[256] = "";
                size_t el = 0;
                for (size_t ei = expr_start; ei < expr_end; ei++) {
                    Token *et = &tokens[ei];
                    if (et->type == TOK_IDENT && strcmp(et->text, "len") == 0
                            && ei + 3 < expr_end
                            && tokens[ei + 1].type == TOK_LBRACKET
                            && tokens[ei + 2].type == TOK_IDENT
                            && tokens[ei + 3].type == TOK_RBRACKET) {
                        el += snprintf(expr + el, sizeof(expr) - el, "%s.size()", tokens[ei + 2].text);
                        ei += 3;
                        continue;
                    }
                    const char *part = "";
                    if (et->type == TOK_IDENT || et->type == TOK_NUMBER) part = et->text;
                    else if (et->type == TOK_PLUS)  part = " + ";
                    else if (et->type == TOK_MINUS) part = " - ";
                    else if (et->type == TOK_MUL)   part = " * ";
                    else if (et->type == TOK_DIV)   part = " / ";
                    else if (et->type == TOK_MOD)   part = " % ";
                    el += snprintf(expr + el, sizeof(expr) - el, "%s", part);
                }
                int idx = loop_idx++;
                write_indent(out, indent);
                fprintf(out,
                    "{ double _rep_%d = %s; for (double _loop_%d = 0; _loop_%d < _rep_%d; _loop_%d++) {\n",
                    idx, expr, idx, idx, idx, idx);
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
                fputs("return ", out);
                emit_string_expr(tokens[i].text, out);
                fputs(";\n", out);
                i++;
            } else {
                fputs("return ", out);
                i = emit_expression(tokens, i, out);
                fputs(";\n", out);
            }
            continue;
        }

        if (tokens[i].type == TOK_END) return;

        fprintf(stderr, "Unknown token at position %zu, type: %d, text: '%s'\n",
                i, tokens[i].type, tokens[i].text);
        exit(1);
    }
}

void convert_to_cpp(Token tokens[], size_t token_count, const char *output_path, CompileStats *stats) {
    sdl_window_created = 0;

    double t0 = 0, t1 = 0;
    if (stats) t0 = get_time_ms();
    parse_structs(tokens, token_count);
    parse_functions(tokens, token_count);
    if (stats) {
        t1 = get_time_ms();
        stats->parse_time = t1 - t0;
    }

    FILE *out = fopen(output_path, "w");
    if (!out) { perror("fopen"); exit(1); }

    fputs(
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <cstring>\n"
        "#include <vector>\n"
        "#include <random>\n"
        "#include <iomanip>\n"
        "#include <cmath>\n"
        "#include <variant>\n"
        "#include <thread>\n"
        "#include <chrono>\n",
        out);
#if HAVE_SDL
    fputs("#include <SDL2/SDL.h>\n", out);
#endif
#if HAVE_SDL_TTF
    fputs("#include <SDL2/SDL_ttf.h>\n", out);
#endif

    fputs(
        "\n"
        "class _KnVal {\n"
        "public:\n"
        "    enum Type { NUMBER, STRING, ARRAY };\n"
        "    Type _type;\n"
        "    double _n;\n"
        "    std::string _t;\n"
        "    std::vector<_KnVal> _a;\n"
        "    \n"
        "    _KnVal() : _type(NUMBER), _n(0) {}\n"
        "    _KnVal(double v) : _type(NUMBER), _n(v) {}\n"
        "    _KnVal(int v) : _type(NUMBER), _n(v) {}\n"
        "    _KnVal(const char* v) : _type(STRING), _n(0), _t(v) {}\n"
        "    _KnVal(const std::string& v) : _type(STRING), _n(0), _t(v) {}\n"
        "    _KnVal(const std::vector<_KnVal>& v) : _type(ARRAY), _n(0), _a(v) {}\n"
        "    \n"
        "    size_t size() const { if (_type == ARRAY) return _a.size(); return 0; }\n"
        "    _KnVal operator[](double idx) const { if (_type == ARRAY && (int)idx < (int)_a.size()) return _a[(int)idx]; return _KnVal(); }\n"
        "    _KnVal& operator[](double idx) { if (_type == ARRAY && (int)idx >= 0 && (int)idx < (int)_a.size()) return _a[(int)idx]; static _KnVal dummy; dummy = _KnVal(); return dummy; }\n"
        "    \n"
        "    operator double() const { return _n; }\n"
        "    operator std::string() const { return _t; }\n"
        "    operator std::vector<_KnVal>() const { return _a; }\n"
        "    \n"
        "    _KnVal& operator+=(double v) { _n += v; return *this; }\n"
        "    _KnVal& operator-=(double v) { _n -= v; return *this; }\n"
        "    _KnVal& operator*=(double v) { _n *= v; return *this; }\n"
        "    _KnVal& operator/=(double v) { _n /= v; return *this; }\n"
        "    _KnVal& operator++() { _n++; return *this; }\n"
        "    _KnVal& operator--() { _n--; return *this; }\n"
        "    _KnVal  operator++(int) { _KnVal _tmp(*this); _n++; return _tmp; }\n"
        "    _KnVal  operator--(int) { _KnVal _tmp(*this); _n--; return _tmp; }\n"
        "    friend bool operator> (const _KnVal& a, const _KnVal& b) { return a._n >  b._n; }\n"
        "    friend bool operator< (const _KnVal& a, const _KnVal& b) { return a._n <  b._n; }\n"
        "    friend bool operator>=(const _KnVal& a, const _KnVal& b) { return a._n >= b._n; }\n"
        "    friend bool operator<=(const _KnVal& a, const _KnVal& b) { return a._n <= b._n; }\n"
        "    friend bool operator==(const _KnVal& a, const _KnVal& b) { if(a._type==_KnVal::STRING&&b._type==_KnVal::STRING) return a._t==b._t; return a._n==b._n; }\n"
        "    friend bool operator!=(const _KnVal& a, const _KnVal& b) { return !(a==b); }\n"
        "    friend bool operator> (const _KnVal& a, double b) { return a._n >  b; }\n"
        "    friend bool operator< (const _KnVal& a, double b) { return a._n <  b; }\n"
        "    friend bool operator>=(const _KnVal& a, double b) { return a._n >= b; }\n"
        "    friend bool operator<=(const _KnVal& a, double b) { return a._n <= b; }\n"
        "    friend bool operator==(const _KnVal& a, double b) { return a._n == b; }\n"
        "    friend bool operator!=(const _KnVal& a, double b) { return a._n != b; }\n"
        "    friend bool operator> (double a, const _KnVal& b) { return a >  b._n; }\n"
        "    friend bool operator< (double a, const _KnVal& b) { return a <  b._n; }\n"
        "    friend bool operator>=(double a, const _KnVal& b) { return a >= b._n; }\n"
        "    friend bool operator<=(double a, const _KnVal& b) { return a <= b._n; }\n"
        "    friend bool operator==(double a, const _KnVal& b) { return a == b._n; }\n"
        "    friend bool operator!=(double a, const _KnVal& b) { return a != b._n; }\n"
        "    \n"
        "    friend std::ostream& operator<<(std::ostream& os, const _KnVal& v) {\n"
        "        if (v._type == STRING) return os << v._t;\n"
        "        if (v._type == ARRAY) {\n"
        "            os << \"[\";\n"
        "            for (size_t _i = 0; _i < v._a.size(); _i++) {\n"
        "                if (_i > 0) os << \", \";\n"
        "                if (v._a[_i]._type == STRING) os << \"\\\"\" << v._a[_i]._t << \"\\\"\";\n"
        "                else if (v._a[_i]._type == ARRAY) os << v._a[_i];\n"
        "                else { std::ostringstream _s; double _d = v._a[_i]._n; if (_d == (long long)_d) _s << (long long)_d; else _s << _d; os << _s.str(); }\n"
        "            }\n"
        "            return os << \"]\";\n"
        "        }\n"
        "        { double _d = v._n; if (_d == (long long)_d) return os << (long long)_d; return os << _d; }\n"
        "    }\n"
        "};\n"
        "using _KnTable = std::vector<_KnVal>;\n"
        "using _KnTable2D = _KnTable;\n"
        "inline std::ostream& operator<<(std::ostream& os, const _KnTable& t) {\n"
        "    os << \"[\";\n"
        "    for (size_t _i = 0; _i < t.size(); _i++) {\n"
        "        if (_i > 0) os << \", \";\n"
        "        if (t[_i]._type == _KnVal::STRING) os << \"\\\"\" << t[_i]._t << \"\\\"\";\n"
        "        else if (t[_i]._type == _KnVal::ARRAY) os << t[_i];\n"
        "        else { std::ostringstream _s; double _d = t[_i]._n; if (_d == (long long)_d) _s << (long long)_d; else _s << _d; os << _s.str(); }\n"
        "    }\n"
        "    return os << \"]\";\n"
        "}\n\n"
        "std::mt19937 _rng(std::random_device{}());\n"
        "double _random(double min, double max) {\n"
        "    std::uniform_real_distribution<double> dist(min, max);\n"
        "    return dist(_rng);\n"
        "}\n"
        "double _lerp(double a, double b, double t) { return a + (b - a) * t; }\n"
        "double _distance(double x1, double y1, double x2, double y2) {\n"
        "    return std::sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));\n"
        "}\n"
        "double _clamp(double x, double min, double max) {\n"
        "    if (x < min) return min;\n"
        "    if (x > max) return max;\n"
        "    return x;\n"
        "}\n"
        "void _delay(double seconds) {\n"
        "    std::this_thread::sleep_for(std::chrono::milliseconds((int)(seconds * 1000)));\n"
        "}\n"
        "double len(_KnTable& table) {\n"
        "    return table.size();\n"
        "}\n\n",
        out);

#if HAVE_SDL
    fputs(
        "SDL_Window *_window = nullptr;\n"
        "SDL_Renderer *_renderer = nullptr;\n"
        "int _key_state[512] = {0};\n"
        "int _prev_key_state[512] = {0};\n"
        "double deltaTime = 0.0;\n"
        "Uint32 _last_frame_time = 0;\n"
        "std::string _font_path = \"\";\n\n"
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
        "        SDL_RenderDrawLine(r, cx-x, cy+y, cx+x, cy+y);\n"
        "        SDL_RenderDrawLine(r, cx-x, cy-y, cx+x, cy-y);\n"
        "        SDL_RenderDrawLine(r, cx-y, cy+x, cx+y, cy+x);\n"
        "        SDL_RenderDrawLine(r, cx-y, cy-x, cx+y, cy-x);\n"
        "        if (err <= 0) { y++; err += 2*y+1; }\n"
        "        else { x--; err -= 2*x+1; }\n"
        "    }\n"
        "}\n\n"
        "#ifdef SDL_TTF_H_\n"
        "void _drawText(SDL_Renderer *r, int x, int y, const char *text, int size,\n"
        "               int red, int green, int blue) {\n"
        "    TTF_Font *font = nullptr;\n"
        "    if (!_font_path.empty()) font = TTF_OpenFont(_font_path.c_str(), size);\n"
        "    if (!font) font = TTF_OpenFont(\"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf\", size);\n"
        "    if (!font) font = TTF_OpenFont(\"/System/Library/Fonts/Arial.ttf\", size);\n"
        "    if (!font) font = TTF_OpenFont(\"C:\\\\\\\\Windows\\\\\\\\Fonts\\\\\\\\arial.ttf\", size);\n"
        "    if (!font) return;\n"
        "    SDL_Color color = {(Uint8)red, (Uint8)green, (Uint8)blue, 255};\n"
        "    SDL_Surface *surf = TTF_RenderText_Solid(font, text, color);\n"
        "    if (surf) {\n"
        "        SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);\n"
        "        SDL_Rect rect = {x, y, surf->w, surf->h};\n"
        "        SDL_RenderCopy(r, tex, nullptr, &rect);\n"
        "        SDL_DestroyTexture(tex);\n"
        "        SDL_FreeSurface(surf);\n"
        "    }\n"
        "    TTF_CloseFont(font);\n"
        "}\n"
        "#else\n"
        "void _drawText(SDL_Renderer *r, int x, int y, const char *text, int size,\n"
        "               int red, int green, int blue)"
        " { (void)r; (void)x; (void)y; (void)text; (void)size; (void)red; (void)green; (void)blue; }\n"
        "#endif\n\n",
        out);
#endif

    for (size_t ci = 0; ci < struct_count; ci++) {
        Struct *c = &structs[ci];
        fprintf(out, "struct %s {\n", c->name);
        for (size_t fi2 = 0; fi2 < c->field_count; fi2++) {
            if (c->field_is_struct[fi2])
                fprintf(out, "    %s %s;\n", c->field_struct_types[fi2], c->field_names[fi2]);
            else if (c->field_is_array[fi2])
                fprintf(out, "    _KnTable %s = %s;\n", c->field_names[fi2], c->field_defaults[fi2]);
            else if (c->field_is_string[fi2])
                fprintf(out, "    std::string %s = %s;\n", c->field_names[fi2], c->field_defaults[fi2]);
            else
                fprintf(out, "    double %s = %s;\n", c->field_names[fi2], c->field_defaults[fi2]);
        }
        for (size_t mi = 0; mi < c->method_count; mi++) {
            Function *m = &c->methods[mi];
            RetType rt = detect_return_type(m->tokens, m->token_count);
            const char *ret_str = rt == RT_STRING ? "std::string" : rt == RT_DOUBLE ? "double" : "void";
            fprintf(out, "    %s %s(", ret_str, m->name);
            for (size_t pi = 0; pi < m->param_count; pi++) {
                if (pi > 0) fputs(", ", out);
                if (param_is_2d_array(m->param_names[pi], m->tokens, m->token_count))
                    fprintf(out, "_KnTable2D& %s", m->param_names[pi]);
                else if (param_is_array(m->param_names[pi], m->tokens, m->token_count))
                    fprintf(out, "_KnTable& %s", m->param_names[pi]);
                else {
                    const char *ct = param_struct_type(m->param_names[pi], m->tokens, m->token_count);
                    if (ct) fprintf(out, "%s& %s", ct, m->param_names[pi]);
                    else    fprintf(out, "_KnVal %s", m->param_names[pi]);
                }
            }
            fputs(") {\n", out);
            convert_tokens_to_cpp(m->tokens, m->token_count, out, 2, 0);
            fputs("    }\n", out);
        }
        fputs("};\n\n", out);
    }

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
                "#ifdef SDL_TTF_H_\n"
                "    TTF_Quit();\n"
                "#endif\n"
                "    SDL_Quit();\n",
                out);
        }
#endif
        fputs("    return 0;\n}\n", out);
    }

    fclose(out);
}
