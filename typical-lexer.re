#include "typical.h"
#include "typical-lexer.h"
#include "typical-parser.h"

#define YYCTYPE     unsigned char
#define YYCURSOR    (tokenizer->start)
#define YYLIMIT     (tokenizer->end)
#define YYFILL(n)
#define YYMARKER    marker

#define RET(num) { token->token_len = tokenizer->start - token->token; \
                   token->id = num; return (num); }

int php_typical_get_token(php_typical_tokenizer *tokenizer, php_typical_token *token) {
	YYCTYPE *YYMARKER = YYCURSOR;
	token->token = YYCURSOR;

/*!re2c
	LABELCHAR   = [0-9a-zA-Z_\200-\377];
	LABELSTART  = [a-zA-Z_\200-\377];
	WHITESPACE  = [ \t\r\n\v\f]+;
	EOF         = [\000];

	[(]                       { RET(TYP_LP); }
	[)]                       { RET(TYP_RP); }
	[|]                       { RET(TYP_PIPE); }
	[&]                       { RET(TYP_AMP); }
	[\^]                      { RET(TYP_CARET); }
	[\?]                      { RET(TYP_QUEST); }
	(LABELSTART LABELCHAR*)   { RET(TYP_LABEL); }
	WHITESPACE                { RET(' '); }
	EOF                       { RET(0); }
*/
}
