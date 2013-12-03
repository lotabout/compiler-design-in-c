/* Revised parser */

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "lex.h"



void factor(void);
void term(void);
void expression(void);
bool legal_lookahead(token_t first_arg,...);

void statements(void)
{
    /* statements -> expression SEMI | expression SEMI statements */
    while (! match(EOI)) {
        expression();

        if (match(SEMI)) {
            advance();
        } else {
            fprintf(stderr, "%d: Inserting missing semicolon\n", yylineno);
        }
    }
}

void expression(void)
{
    /* expression -> term expression'
     * expression' -> PLUS term expression' | epsilon */
    if (! legal_lookahead(NUM_OR_ID, LP, 0)) {
        return;
    }

    term();
    while (match(PLUS)) {
        advance();
        term();
    }
}

void term(void)
{
    /* term -> factor term' 
     * term' -> TIMES factor term'
     *       |  epsilon
     */
    if (! legal_lookahead(NUM_OR_ID, LP, 0)) {
        return;
    }

    factor();
    while (match(TIMES)) {
        advance();
        factor();
    }
}

void factor(void)
{
    /* factor -> NUM_OR_ID
     *        |  LP expression RP
     */
    if (! legal_lookahead(NUM_OR_ID, LP, 0)) {
        return;
    }

    if (match(NUM_OR_ID)) {
        advance();
    } else if (match(LP)) {
        advance();
        expression();
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
