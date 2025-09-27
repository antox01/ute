#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_END = 0,
    TOKEN_TYPES,
    TOKEN_KEYWORD,
    TOKEN_LITERAL,
    TOKEN_SYMBOL,
    TOKEN_PREPROC,
    TOKEN_COMMENT,
    TOKEN_INVALID,
    TOKEN_COUNT,
} Token_Kind;

typedef struct {
    Token_Kind kind;
    size_t start;
    size_t count;
} Token;

typedef struct {
    char *data;
    size_t cursor;
    size_t size;

    Token token;
} Lexer;

Lexer lexer_init(char *data, size_t size);
void lexer_next(Lexer *l);

#endif// LEXER_H
