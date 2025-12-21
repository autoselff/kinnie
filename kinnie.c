#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 256
#define MAX_VARS   64
#define MAX_FUNCTIONS 32
#define MAX_SCOPE_DEPTH 16
#define MAX_NAME_LEN 32
#define MAX_STRING_LEN 128
#define MAX_FUNC_PARAMS 8

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
    TOK_UNKNOWN
} TokenType;

typedef enum {
    VAR_DOUBLE,
    VAR_STRING
} VarType;

typedef struct {
    TokenType type;
    char text[MAX_STRING_LEN];
} Token;

typedef struct {
    char name[MAX_NAME_LEN];
    VarType var_type;
    union {
        double double_value;
        char string_value[MAX_STRING_LEN];
    };
} Variable;

typedef struct {
    Variable vars[MAX_VARS];
    size_t var_count;
    int is_function_boundary;
} Scope;

typedef struct {
    char name[MAX_NAME_LEN];
    Token tokens[MAX_TOKENS];
    size_t token_count;
    char param_names[MAX_FUNC_PARAMS][MAX_NAME_LEN];
    size_t param_count;
} Function;

FILE* c_output = NULL;
int c_indent = 0;

Function functions[MAX_FUNCTIONS];
size_t function_count = 0;

Scope scope_stack[MAX_SCOPE_DEPTH];
size_t scope_depth = 0;

Variable return_value;
int has_return_value = 0;

void push_scope(int is_function) {
    if (scope_depth >= MAX_SCOPE_DEPTH) {
        fprintf(stderr, "Scope depth exceeded\n");
        exit(1);
    }
    scope_stack[scope_depth].var_count = 0;
    scope_stack[scope_depth].is_function_boundary = is_function;
    scope_depth++;
}

void pop_scope() {
    if (scope_depth == 0) {
        fprintf(stderr, "Scope underflow\n");
        exit(1);
    }
    scope_depth--;
}

Variable *get_var(const char *name) {
    for (int i = scope_depth - 1; i >= 0; i--) {
        for (size_t j = 0; j < scope_stack[i].var_count; j++) {
            if (strcmp(scope_stack[i].vars[j].name, name) == 0)
                return &scope_stack[i].vars[j];
        }

        if (scope_stack[i].is_function_boundary) break;
    }
    return NULL;
}

void set_var_double(const char *name, double value) {
    Variable *v = get_var(name);
    if (v) {
        v->var_type = VAR_DOUBLE;
        v->double_value = value;
        return;
    }
    
    Scope *current = &scope_stack[scope_depth - 1];
    strncpy(current->vars[current->var_count].name, name, MAX_NAME_LEN - 1);
    current->vars[current->var_count].var_type = VAR_DOUBLE;
    current->vars[current->var_count].double_value = value;
    current->var_count++;
}

void set_var_string(const char *name, const char *value) {
    Variable *v = get_var(name);
    if (v) {
        v->var_type = VAR_STRING;
        strncpy(v->string_value, value, MAX_STRING_LEN - 1);
        v->string_value[MAX_STRING_LEN - 1] = 0;
        return;
    }
    
    Scope *current = &scope_stack[scope_depth - 1];
    strncpy(current->vars[current->var_count].name, name, MAX_NAME_LEN - 1);
    current->vars[current->var_count].var_type = VAR_STRING;
    strncpy(current->vars[current->var_count].string_value, value, MAX_STRING_LEN - 1);
    current->vars[current->var_count].string_value[MAX_STRING_LEN - 1] = 0;
    current->var_count++;
}

size_t tokenize(const char *src, Token tokens[]) {
    size_t i = 0, t = 0;
    while (src[i]) {
        if (isspace(src[i])) {
            i++;
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
        if (isdigit(src[i])) {
            size_t start = i;
            while (isdigit(src[i])) i++;

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

double parse_value(Token *tok) {
    if (tok->type == TOK_NUMBER)
        return atof(tok->text);
    if (tok->type == TOK_IDENT) {
        Variable *v = get_var(tok->text);
        if (!v) {
            fprintf(stderr, "Unknown variable: %s\n", tok->text);
            exit(1);
        }
        if (v->var_type != VAR_DOUBLE) {
            fprintf(stderr, "Variable %s is not a number\n", tok->text);
            exit(1);
        }
        return v->double_value;
    }
    fprintf(stderr, "Syntax error\n");
    exit(1);
}

double evaluate_expression(Token tokens[], size_t *idx) {
    double result = parse_value(&tokens[*idx]);
    (*idx)++;

    while (tokens[*idx].type == TOK_PLUS ||
        tokens[*idx].type == TOK_MINUS ||
        tokens[*idx].type == TOK_MUL ||
        tokens[*idx].type == TOK_DIV ||
        tokens[*idx].type == TOK_MOD) {

        TokenType op = tokens[*idx].type;
        (*idx)++;

        double rhs = parse_value(&tokens[*idx]);
        (*idx)++;

        switch (op) {
            case TOK_PLUS: result += rhs; break; 
            case TOK_MINUS: result -= rhs; break;
            case TOK_MUL: result *= rhs; break;
            case TOK_DIV: result /= rhs; break;
            case TOK_MOD: result = (double)((int)result % (int)rhs); break;
            default: break;
        }
    }
    return result;
}

void interpret_tokens(Token tokens[], size_t token_count);

void call_function(const char *name, double *args, size_t arg_count) {
    Function *func = get_function(name);
    if (!func) {
        fprintf(stderr, "Unknown function: %s\n", name);
        exit(1);
    }

    if (arg_count != func->param_count) {
        fprintf(stderr, "The arguments do not match. Expected %zu, got %zu\n", 
                func->param_count, arg_count);
        exit(1);
    }

    has_return_value = 0;

    push_scope(1);
    
    for (size_t i = 0; i < func->param_count; i++) {
        set_var_double(func->param_names[i], args[i]);
    }
    
    interpret_tokens(func->tokens, func->token_count);
    pop_scope();
}

void interpret_tokens(Token tokens[], size_t token_count) {
    size_t i = 0;
    while (i < token_count && tokens[i].type != TOK_EOF) {
        if (tokens[i].type == TOK_VAR) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i + 1].text);
            i += 3;

            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
                char func_name[MAX_NAME_LEN];
                strcpy(func_name, tokens[i].text);
                i += 2;
                
                double args[MAX_FUNC_PARAMS];
                size_t arg_count = 0;
                
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    args[arg_count++] = evaluate_expression(tokens, &i);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                
                if (tokens[i].type != TOK_RBRACKET) {
                    fprintf(stderr, "Expected ')'\n");
                    exit(1);
                }
                i++;
                
                call_function(func_name, args, arg_count);
                
                if (has_return_value) {
                    if (return_value.var_type == VAR_DOUBLE) {
                        set_var_double(name, return_value.double_value);
                    } else if (return_value.var_type == VAR_STRING) {
                        set_var_string(name, return_value.string_value);
                    }
                } else {
                    fprintf(stderr, "Function %s did not return a value\n", func_name);
                    exit(1);
                }
                
                continue;
            }
            
            if (tokens[i].type == TOK_STRING) {
                set_var_string(name, tokens[i].text);
                fprintf(c_output, "char %s[%d] = \"%s\";\n", name, MAX_STRING_LEN, tokens[i].text);
                i++;
            } else {
                double value = evaluate_expression(tokens, &i);
                set_var_double(name, value);
                fprintf(c_output, "double %s = %lf;\n", name, value);
            }
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_ASSIGN) {
            char name[MAX_NAME_LEN];
            strcpy(name, tokens[i].text);
            i += 2;
            
            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
                char func_name[MAX_NAME_LEN];
                strcpy(func_name, tokens[i].text);
                i += 2;
                
                double args[MAX_FUNC_PARAMS];
                size_t arg_count = 0;
                
                while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                    args[arg_count++] = evaluate_expression(tokens, &i);
                    if (tokens[i].type == TOK_COMMA) i++;
                }
                i++;
                
                call_function(func_name, args, arg_count);
                
                if (has_return_value) {
                    if (return_value.var_type == VAR_DOUBLE) {
                        set_var_double(name, return_value.double_value);
                    } else if (return_value.var_type == VAR_STRING) {
                        set_var_string(name, return_value.string_value);
                    }
                } else {
                    fprintf(stderr, "Function %s did not return a value\n", func_name);
                    exit(1);
                }
                
                continue;
            }
            
            if (tokens[i].type == TOK_STRING) {
                set_var_string(name, tokens[i].text);
                i++;
            } else {
                double value = evaluate_expression(tokens, &i);
                set_var_double(name, value);
            }
            continue;
        }

        if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_LBRACKET) {
            char func_name[MAX_NAME_LEN];
            strcpy(func_name, tokens[i].text);
            i += 2;
            
            double args[MAX_FUNC_PARAMS];
            size_t arg_count = 0;
            
            while (tokens[i].type != TOK_RBRACKET && tokens[i].type != TOK_EOF) {
                if (arg_count >= MAX_FUNC_PARAMS) {
                    fprintf(stderr, "Too many arguments\n");
                    exit(1);
                }
                
                args[arg_count++] = evaluate_expression(tokens, &i);
                
                if (tokens[i].type == TOK_COMMA) {
                    i++;
                }
            }
            
            if (tokens[i].type != TOK_RBRACKET) {
                fprintf(stderr, "Expected ')'\n");
                exit(1);
            }
            i++;
            
            call_function(func_name, args, arg_count);
            continue;
        }

        if (tokens[i].type == TOK_PRINT) {
            TokenType print_type = tokens[i].type;
            i++;
            
            if (tokens[i].type == TOK_STRING) {
                char *str = tokens[i].text;
                for (size_t j = 0; str[j] != '\0'; j++) {
                    if (str[j] == '\\' && str[j+1] == 'n') {
                        putchar('\n');
                        j++;
                    } else if (str[j] == '{') {
                        j++;
                        char var_name[MAX_NAME_LEN];
                        size_t var_name_i = 0;

                        while (str[j] != '}' && str[j] != '\0') {
                            var_name[var_name_i++] = str[j++];
                        }
                        var_name[var_name_i] = '\0';

                        if (str[j] != '}') {
                            fprintf(stderr, "Missing closing '}' in string interpolation\n");
                            exit(1);
                        }

                        Variable *temp_var = get_var(var_name);
                        if (!temp_var) {
                            fprintf(stderr, "Variable not found: %s\n", var_name);
                            exit(1);
                        }

                        switch (temp_var->var_type) {
                            case VAR_DOUBLE:
                                printf("%.1lf", temp_var->double_value);
                                break;
                            case VAR_STRING:
                                printf("%s", temp_var->string_value);
                                break;
                            default:
                                fprintf(stderr, "Cannot interpolate variable of this type\n");
                                exit(1);
                        }
                    }else {
                        printf("%c", str[j]);
                    }
                }
                
                i++;
                continue;
            }
            
            if (tokens[i].type == TOK_IDENT) {
                Variable *v = get_var(tokens[i].text);
                if (v && v->var_type == VAR_STRING) {
                    char *str = v->string_value;
                    for (size_t j = 0; str[j] != '\0'; j++) {
                        if (str[j] == '\\' && str[j+1] == 'n') {
                            putchar('\n');
                            j++;
                        } else {
                            printf("%c", str[j]);
                        }
                    }
                    
                    i++;
                    continue;
                }
            }
            
            double result = evaluate_expression(tokens, &i);
            printf("%.1lf", result);

            continue;
        }

        if (tokens[i].type == TOK_IF_START) {
            i++;

            double left = 0;
            if (tokens[i].type == TOK_NUMBER) {
                left = atof(tokens[i].text);
            } else if (tokens[i].type == TOK_IDENT) {
                Variable *v = get_var(tokens[i].text);
                if (!v || v->var_type != VAR_DOUBLE) {
                    fprintf(stderr, "Variable not found or not double: %s\n", tokens[i].text);
                    exit(1);
                }
                left = v->double_value;
            }

            int condition_met = 0;
            TokenType op = tokens[i + 1].type;
            if (op == TOK_EQUALS || op == TOK_MORE || op == TOK_LESS || op == TOK_NOT_EQUALS || op == TOK_MORE_EQUALS || op == TOK_LESS_EQUALS) {
                i += 2;

                double right = 0;
                if (tokens[i].type == TOK_NUMBER) {
                    right = atof(tokens[i].text);
                } else if (tokens[i].type == TOK_IDENT) {
                    Variable *v = get_var(tokens[i].text);
                    if (!v || v->var_type != VAR_DOUBLE) {
                        fprintf(stderr, "Variable not found or not double: %s\n", tokens[i].text);
                        exit(1);
                    }
                    right = v->double_value;
                }

                i++;
                if (op == TOK_EQUALS) condition_met = (left == right);
                else if (op == TOK_MORE) condition_met = (left > right);
                else if (op == TOK_LESS) condition_met = (left < right);
                else if (op == TOK_NOT_EQUALS) condition_met = (left != right);
                else if (op == TOK_MORE_EQUALS) condition_met = (left >= right);
                else if (op == TOK_LESS_EQUALS) condition_met = (left <= right);

                if (tokens[i].type != TOK_LBRACE) {
                    fprintf(stderr, "Expected '{' after if condition\n");
                    exit(1);
                }
                i++;
            } else {
                condition_met = (left != 0);
                
                if (tokens[i + 1].type != TOK_LBRACE) {
                    fprintf(stderr, "Expected '{' after if condition\n");
                    exit(1);
                }
                i += 2;
            }

            size_t block_start = i;
            size_t block_end = i;
            int depth = 1;
            
            while (depth > 0 && tokens[block_end].type != TOK_EOF) {
                if (tokens[block_end].type == TOK_LBRACE) depth++;
                if (tokens[block_end].type == TOK_RBRACE) depth--;
                if (depth > 0) block_end++;
            }

            Token if_tokens[MAX_TOKENS];
            size_t if_token_count = 0;
            for (size_t j = block_start; j < block_end; j++) {
                if_tokens[if_token_count++] = tokens[j];
            }
            if_tokens[if_token_count].type = TOK_EOF;

            if (condition_met) {
                push_scope(0);
                interpret_tokens(if_tokens, if_token_count);
                pop_scope();
            }
            
            i = block_end + 1;

            if (i < token_count && tokens[i].type == TOK_ELSE) {
                i++;
                
                if (tokens[i].type != TOK_LBRACE) {
                    fprintf(stderr, "Expected '{' after else\n");
                    exit(1);
                }
                i++;

                size_t else_start = i;
                size_t else_end = i;
                depth = 1;
                
                while (depth > 0 && tokens[else_end].type != TOK_EOF) {
                    if (tokens[else_end].type == TOK_LBRACE) depth++;
                    if (tokens[else_end].type == TOK_RBRACE) depth--;
                    if (depth > 0) else_end++;
                }

                Token else_tokens[MAX_TOKENS];
                size_t else_token_count = 0;
                for (size_t j = else_start; j < else_end; j++) {
                    else_tokens[else_token_count++] = tokens[j];
                }
                else_tokens[else_token_count].type = TOK_EOF;

                if (!condition_met) {
                    push_scope(0);
                    interpret_tokens(else_tokens, else_token_count);
                    pop_scope();
                }
                
                i = else_end + 1;
            }

            continue;
        }

        if (tokens[i].type == TOK_LOOP_START) {
            i++;
            
            char counter_name[MAX_NAME_LEN];
            strcpy(counter_name, tokens[i].text);
            i++;
            
            Variable *counter_var = get_var(counter_name);
            if (!counter_var || counter_var->var_type != VAR_DOUBLE) {
                fprintf(stderr, "Loop counter not found or not int: %s\n", counter_name);
                exit(1);
            }

            size_t goal = counter_var->double_value;
            
            if (tokens[i].type != TOK_LBRACE) {
                fprintf(stderr, "Expected '{' after repeat\n");
                exit(1);
            }
            i++;

            size_t loop_start = i;
            size_t loop_end = i;
            int depth = 1;
            
            while (depth > 0 && tokens[loop_end].type != TOK_EOF) {
                if (tokens[loop_end].type == TOK_LBRACE) depth++;
                if (tokens[loop_end].type == TOK_RBRACE) depth--;
                if (depth > 0) loop_end++;
            }

            Token loop_tokens[MAX_TOKENS];
            size_t loop_token_count = 0;
            for (size_t j = loop_start; j < loop_end; j++) {
                loop_tokens[loop_token_count++] = tokens[j];
            }
            loop_tokens[loop_token_count].type = TOK_EOF;

            counter_var->double_value = 0;
            
            while (counter_var->double_value < goal) {
                push_scope(0);
                interpret_tokens(loop_tokens, loop_token_count);
                pop_scope();
                counter_var->double_value++;
            }
            
            i = loop_end + 1;
            continue;
        }

        if (tokens[i].type == TOK_RETURN) {
            i++;
            
            if (tokens[i].type == TOK_STRING) {
                return_value.var_type = VAR_STRING;
                strncpy(return_value.string_value, tokens[i].text, MAX_STRING_LEN - 1);
                return_value.string_value[MAX_STRING_LEN - 1] = 0;
                has_return_value = 1;
                i++;
            } else {
                double result = evaluate_expression(tokens, &i);
                return_value.var_type = VAR_DOUBLE;
                return_value.double_value = result;
                has_return_value = 1;
            }
            
            return;
        }

        if (tokens[i].type == TOK_END) {
            return;
        }

        fprintf(stderr, "Unknown command at position %zu, token type: %d\n", i, tokens[i].type);
        exit(1);
    }
}

void parse_functions(Token tokens[], size_t token_count) {
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
            
            while (depth > 0 && tokens[fun_end].type != TOK_EOF) {
                if (tokens[fun_end].type == TOK_LBRACE) depth++;
                if (tokens[fun_end].type == TOK_RBRACE) depth--;
                if (depth > 0) fun_end++;
            }

            strncpy(functions[function_count].name, func_name, MAX_NAME_LEN - 1);
            functions[function_count].token_count = 0;
            
            for (size_t j = fun_start; j < fun_end; j++) {
                functions[function_count].tokens[functions[function_count].token_count++] = tokens[j];
            }
            functions[function_count].tokens[functions[function_count].token_count].type = TOK_EOF;
            
            function_count++;
            i = fun_end + 1;
            continue;
        }
        i++;
    }
}

void interpret(Token tokens[], size_t token_count) {
    parse_functions(tokens, token_count);

    Function *main_func = get_function("main");
    if (!main_func) {
        fprintf(stderr, "No 'main' function found\n");
        exit(1);
    }

    double no_args[1] = {0};
    fprintf(c_output, "int main(void) {\n");
    call_function("main", no_args, 0);
    fprintf(c_output, "}\n");
}

int has_extension(const char *name, const char *ext) {
    size_t nlen = strlen(name);
    size_t elen = strlen(ext);
    if (nlen < elen) return 0;
    return strcmp(name + nlen - elen, ext) == 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file.kn\n", argv[0]);
        return 1;
    }

    char output_name[256];
    size_t len = strlen(argv[1]);

    if (len > 3 && strcmp(argv[1] + len - 3, ".kn") == 0) {
        snprintf(output_name, sizeof(output_name), "%.*s", (int)(len - 3), argv[1]);
    } else {
        snprintf(output_name, sizeof(output_name), "%s", argv[1]);
    }
    
    char c_filename[512];
    snprintf(c_filename, sizeof(c_filename), "%s.c", output_name);
    c_output = fopen(c_filename, "w");

    fprintf(c_output, "#include <stdio.h>\n#include <string.h>\n#include <ctype.h>\n#include <string.h>\n");

    Token tokens[MAX_TOKENS];
    size_t token_count;

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
    interpret(tokens, token_count);

    fclose(c_output);
    free(source);

    char cmd[1024];
    
    snprintf(cmd, sizeof(cmd), "gcc -o %s %s.c", output_name, output_name);
    system(cmd);
    
    // snprintf(cmd, sizeof(cmd), "rm %s.c", output_name);
    // system(cmd);
    
    snprintf(cmd, sizeof(cmd), "./%s", output_name);
    system(cmd);

    return 0;
}