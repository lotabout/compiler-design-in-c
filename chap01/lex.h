typedef enum {
    EOI       = 0, /* end of input */
    SEMI      = 1, /* ; */
    PLUS      = 2, /* + */
    TIMES     = 3, /* * */
    LP        = 4, /* ( */
    RP        = 5, /* ) */
    NUM_OR_ID = 6, /* decimal number or identifier */
    UNKNOWN,
} token_t;

extern char *yytext;    /* in lex.c */
extern int yyleng;
extern int yylineno;
