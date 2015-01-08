/* Revised parser */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "lex.h"

void *factor(char *tempvar);
void *term(char *tempvar);
void *expression(char *tempvar);
bool legal_lookahead(token_t first_arg,...);
extern char *newname(void);
extern void freename(char *name);

void statements(void)
{
    /* statements -> expression SEMI | expression SEMI statements */
    char *tempvar;
    while (! match(EOI)) {
        expression(tempvar = newname());
        freename(tempvar);

        if (match(SEMI)) {
            advance();
        } else {
            fprintf(stderr, "%d: Inserting missing semicolon\n", yylineno);
        }

        freename(tempvar);
    }
}

void *expression(char *tempvar)
{
    /* expression -> term expression'
     * expression' -> PLUS term expression' | epsilon */
    char *tempvar2;

    term(tempvar);
    while (match(PLUS)) {
        advance();
        term(tempvar2 = newname());
        printf("    %s += %s\n", tempvar, tempvar2);
        freename(tempvar2);
    }

    return tempvar;
}

void *term(char *tempvar)
{
    /* term -> factor term' 
     * term' -> TIMES factor term'
     *       |  epsilon
     */
    char *tempvar2;

    factor(tempvar);
    while (match(TIMES)) {
        advance();
        factor(tempvar2 = newname());
        printf("    %s *= %s\n", tempvar, tempvar2);
        freename(tempvar2);
    }
}

void *factor(char *tempvar)
{
    /* factor -> NUM_OR_ID
     *        |  LP expression RP
     */

    if (match(NUM_OR_ID)) {
        printf("    %s = %.*s\n", tempvar, yyleng, yytext);
        advance();
    } else if (match(LP)) {
        advance();
        expression(tempvar);
        if (match(RP)) {
            advance();
        } else {
            fprintf(stderr, "%d: Mismatched parenthesis\n", yylineno);
        }
    } else {
        fprintf(stderr, "%d: Number of identifier expected\n", yylineno);
    }
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
