#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "lexer.h"

static const char *c_types[] = { "int", "float", "double", "char", "void", "size_t", };

static const char *c_keywords[] = {
    "const", "if", "else", "for", "while", "do", "return", "switch", "case", "default",
    "typedef", "struct", "break", "continue",
};

bool is_symbol_start(char ch) {
    return ch == '_' || isalpha(ch);
}

bool is_symbol(char ch) {
    return ch == '_' || isalnum(ch);
}

char *token_str(Token_Kind tk) {
    switch(tk) {
        case TOKEN_END: return "TOKEN_END";
        case TOKEN_TYPES: return "TOKEN_TYPES";
        case TOKEN_KEYWORD: return "TOKEN_KEYWORD";
        case TOKEN_LITERAL: return "TOKEN_LITERAL";
        case TOKEN_SYMBOL: return "TOKEN_SYMBOL";
        case TOKEN_PREPROC: return "TOKEN_PREPROC";
        case TOKEN_COMMENT: return "TOKEN_COMMENT";
        case TOKEN_INVALID: return "TOKEN_INVALID";
        case TOKEN_COUNT: return "TOKEN_COUNT";
    }
    return "Unreachable";
}
Lexer lexer_init(char *data, size_t size) {
    Lexer lexer = {
        .data = data,
        .size = size,
        .cursor = 0,
    };
    return lexer;
}

void lexer_trim_left(Lexer *l) {
    while(isspace(l->data[l->cursor])) l->cursor++;
}

/*
 * Check if the current token to analyze in the lexer starts with
 * the passed string. NOTE: the characters will be consumed by the function.
 */
bool lexer_starts_with_cstr(Lexer *l, char *start) {
    int saved_cursor = l->cursor;
    while(*start && l->cursor < l->size && l->data[l->cursor] == *start) {
        start++;
        l->cursor++;
    }
    if(*start) {
        l->cursor = saved_cursor;
        return false;
    }
    return true;
}

void lexer_next(Lexer *l) {
    lexer_trim_left(l);

    l->token.kind = TOKEN_INVALID;
    l->token.start = l->cursor;

    for(size_t kw = 0; kw < ARRAY_LEN(c_keywords); kw++) {
        int saved_cursor = l->cursor;
        const char *keyword = c_keywords[kw];
        size_t keyword_len = strlen(c_keywords[kw]);
        if(l->cursor+keyword_len <= l->size && strncmp(&l->data[l->cursor], keyword, keyword_len) == 0) {
            l->cursor+=keyword_len;
            if(l->cursor < l->size && is_symbol(l->data[l->cursor])) {
                l->cursor = saved_cursor;
            } else {
                l->token .count = keyword_len;
                l->token.kind = TOKEN_KEYWORD;
                return;
            }
        }
    }

    for(size_t tp = 0; tp < ARRAY_LEN(c_types); tp++) {
        int saved_cursor = l->cursor;
        const char *type = c_types[tp];
        size_t type_len = strlen(c_types[tp]);
        if(l->cursor+type_len <= l->size && strncmp(&l->data[l->cursor], type, type_len) == 0) {
            l->cursor+=type_len;
            if(l->cursor < l->size && is_symbol(l->data[l->cursor])) {
                l->cursor = saved_cursor;
            } else {
                l->token.count = type_len;
                l->token.kind = TOKEN_TYPES;
                return;
            }
        }
    }

    if(l->data[l->cursor] == '"') {
        int saved_cursor = l->cursor;
        l->cursor++;
        while(l->cursor < l->size && l->data[l->cursor] != '"') l->cursor++;
        if(l->cursor < l->size) l->cursor++;
        // NOTE: in case there is no `"` to close the string,
        // just set everything as a string
        l->token.count = l->cursor - saved_cursor;
        l->token.kind = TOKEN_LITERAL;
        return;
    }

    if(l->data[l->cursor] == '\'') {
        int saved_cursor = l->cursor;
        l->cursor++;
        if(l->data[l->cursor] == '\\') l->cursor++;
        l->cursor++;
        if(l->data[l->cursor] == '\'') {
            l->cursor++;
            // NOTE: in case there is no `'` to close the string,
            // just set everything as a string
            l->token.count = l->cursor - saved_cursor;
            l->token.kind = TOKEN_LITERAL;
            return;
        }
        l->cursor = saved_cursor;
    }

    if(is_symbol_start(l->data[l->cursor])) {
        while(l->cursor < l->size && is_symbol(l->data[l->cursor])) l->cursor++;
        l->token.kind = TOKEN_SYMBOL;
        l->token.count = l->cursor - l->token.start;
        return;
    }

    if(isdigit(l->data[l->cursor])) {
        if(l->data[l->cursor] == '0') {
            l->cursor++;
            switch(l->data[l->cursor]) {
                case 'b':
                case 'o':
                case 'x':
                    l->cursor++;
                    break;
                default:
                    break;
            }
        }
        while(l->cursor < l->size && isdigit(l->data[l->cursor])) l->cursor++;
        l->token.kind = TOKEN_LITERAL;
        l->token.count = l->cursor - l->token.start;
        return;
    }

    if(l->data[l->cursor] == '#') {
        while(l->cursor < l->size && l->data[l->cursor] != '\n') {
            l->cursor++;
        }
        if(l->cursor < l->size) l->cursor++;

        l->token.count = l->cursor - l->token.start;
        l->token.kind = TOKEN_PREPROC;
        return;
    }

    if(lexer_starts_with_cstr(l, "//")) {
        while(l->cursor < l->size && l->data[l->cursor] != '\n') {
            l->cursor++;
        }
        if(l->cursor < l->size) l->cursor++;

        l->token.count = l->cursor - l->token.start;
        l->token.kind = TOKEN_COMMENT;
        return;
    }

    if(lexer_starts_with_cstr(l, "/*")) {
        while(l->cursor < l->size && !lexer_starts_with_cstr(l, "*/")) {
            l->cursor++;
        }
        l->token.count = l->cursor - l->token.start;
        l->token.kind = TOKEN_COMMENT;
        return;
    }

    l->cursor++;
    l->token.count = 1;

    return;
}

