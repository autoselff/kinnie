#ifndef KINNIE_H
#define KINNIE_H

#include <stddef.h>

#ifdef __has_include
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#    define HAVE_SDL 1
#  else
#    define HAVE_SDL 0
#  endif
#  if __has_include(<SDL2/SDL_ttf.h>)
#    include <SDL2/SDL_ttf.h>
#    define HAVE_SDL_TTF 1
#  else
#    define HAVE_SDL_TTF 0
#  endif
#else
#  define HAVE_SDL 0
#  define HAVE_SDL_TTF 0
#endif

#define KINNIE_VERSION      "3.0.1"
#define MAX_TOKENS          2048
#define MAX_EXPANDED_TOKENS 8192
#define MAX_FUNCTIONS       64
#define MAX_NAME_LEN        32
#define MAX_STRING_LEN      128
#define MAX_FUNC_PARAMS     16
#define ARG_BUF_LEN         256
#define MAX_STRUCTS         32
#define MAX_STRUCT_FIELDS   32
#define MAX_STRUCT_METHODS  16

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
    TOK_RETURN, TOK_KEY_PRESSED, TOK_KEY_DOWN, TOK_ADD, TOK_STOP, TOK_STRUCT,
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

typedef struct {
    char name[MAX_NAME_LEN];
    char field_names[MAX_STRUCT_FIELDS][MAX_NAME_LEN];
    char field_defaults[MAX_STRUCT_FIELDS][MAX_STRING_LEN];
    int field_is_string[MAX_STRUCT_FIELDS];
    int field_is_array[MAX_STRUCT_FIELDS];
    size_t field_count;
    Function methods[MAX_STRUCT_METHODS];
    size_t method_count;
} Struct;

typedef struct {
    double tokenize_time;
    double includes_time;
    double parse_time;
    double codegen_time;
    double compile_time;
} CompileStats;

#endif
