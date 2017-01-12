#ifndef incl_PHP_TYPICAL_LEXER_H
#define incl_PHP_TYPICAL_LEXER_H

typedef struct _php_typical_token {
	unsigned short id;
	unsigned char *token;
	int token_len;
} php_typical_token;

typedef struct _php_typical_tokenizer {
	unsigned char *start, *end;
} php_typical_tokenizer;

#endif
