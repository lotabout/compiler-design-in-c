/* Revised parser */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "lex.h"

char *factor(void);
char *term(void);
char *expression(void);
bool legal_lookahead(token_t first_arg,...);
extern char *newname(void);
extern void freename(char *name);

void statements(void)
{
    /* statements -> expression SEMI | expression SEMI statements */
    char *tempvar;
    while (! match(EOI)) {
        tempvar = expression();

        if (match(SEMI)) {
            advance();
        } else {
            fprintf(stderr, "%d: Inserting missing semicolon\n", yylineno);
        }

        freename(tempvar);
    }
}

char *expression(void)
{
    /* expression -> term expression'
     * expression' -> PLUS term expression' | epsilon */
    char *tempvar, *tempvar2;

    tempvar = term();
    while (match(PLUS)) {
        advance();
        tempvar2 = term();
        printf("    %s += %s\n", tempvar, tempvar2);
        freename(tempvar2);
    }

    return tempvar;
}

char *term(void)
{
    /* term -> factor term' 
     * term' -> TIMES factor term'
     *       |  epsilon
     */
    char *tempvar, *tempvar2;

    tempvar = factor();
    while (match(TIMES)) {
        advance();
        tempvar2 = factor();
        printf("    %s *= %s\n", tempvar, tempvar2);
        freename(tempvar2);
    }

    return tempvar;
}

char *factor(void)
{
    /* factor -> NUM_OR_ID
     *        |  LP expression RP
     */
    char *tempvar;

    if (match(NUM_OR_ID)) {
        /* print the assignment instruction. The %0.*s conversion is a form of
         * %X.Ys, where X is the field width and Y is the maximum number of
         * characters that will be printed (even if the string is longer). I'm
         * using the %0.*s to print the string because it's not \0 terminated.
         * The field has a default width of 0, but it will grow the size
         * needed to print the string. The ".*" tells printf() to take the
         * maximum-number-of-characters count from the next argument(yyleng) */
        printf("    %s = %.*s\n", tempvar = newname(), yyleng, yytext);
        advance();
    } else if (match(LP)) {
        advance();
        tempvar = expression();
        if (match(RP)) {
            advance();
        } else {
            fprintf(stderr, "%d: Mismatched parenthesis\n", yylineno);
        }
    } else {
        fprintf(stderr, "%d: Number of identifier expected\n", yylineno);
    }

    return tempvar;
}

#define MAXFIRST 16
#define SYNCH   SEMI
bool legal_lookahead(token_t first_arg,...)
{
    /* simple error detection and recovery. Arguments are a 0-terminated list
     * of those token that can legitimately come next in the input. If the
     * list is empty, the end of file must come next. Print an error message
     * if necessary. Error recovery is performed by discarding all input
     * symboals until one that's in the input list is found
     *
     * Return treu if there's no error or if we recovered from the error,
     * false if we can't recover.
     */
    va_list args;
    token_t tok;
    token_t lookaheads[MAXFIRST], *p = lookaheads, *current;
    int error_printed = 0;
    int ret_val = 0;

    va_start(args, first_arg);

    if (! first_arg) {
        if (match(EOF)) {
            ret_val = true;
        }
    } else {
        *p++ = first_arg;
        while ((tok = va_arg(args, token_t)) && p < &lookaheads[MAXFIRST]) {
            *++p = tok;
        }
        while (!match(SYNCH)) {
            for(current = lookaheads; current < p; ++current) {
                if (match(*current)) {
                    ret_val = true;
                    goto exit;
                }
            }

            if (! error_printed) {
                fprintf(stderr, "Line %d: Syntax error\n", yylineno);
                error_printed = 1;
            }

            advance();
        }
    }

exit:
    va_end(args);
    return ret_val;
}
