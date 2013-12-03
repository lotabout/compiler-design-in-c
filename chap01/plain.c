/* Basic parser, shows the structure but there's no code generation */

#include <stdio.h>
#include "lex.h"

void statements(void);
void expression(void);
void expr_prime(void);
void term(void);
void term_prime(void);
void factor(void);

void statements(void)
{
    /* statements -> expression SEMI
     *            |  expression SEMI statements
     */
    expression();

    if (match(SEMI)) {
        advance();
    } else {
        fprintf(stderr, "%d: Inserting missing semicolon\n", yylineno);
    }

    if (! match(EOI)) {
        statements();
    }
}

void expression(void)
{
    /* expression -> term expression' */
    term();
    expr_prime();
}

void expr_prime(void)
{
    /* expression' -> PLUS term expression'
     *             |  epsilon
     */

    if (match(PLUS)) {
        advance();
        term();
        expr_prime();
    }
}

void term(void)
{
    /* term -> factor term' */
    factor();
    term_prime();
}

void term_prime(void)
{
    /* term' -> TIMES factor term'
     *       |  epsilon
     */
    if (match(TIMES)) {
        advance();
        factor();
        term_prime();
    }
}

void factor(void)
{
    /* factor -> NUM_OR_ID
     *        |  LP expression RP
     */
    if (match(NUM_OR_ID)) {
        advance();
    } else if (match(LP)) {
        advance();
        expression();
        if (match (RP)) {
            advance();
        } else {
            fprintf(stderr, "%d: Mismatched parenthesis\n", yylineno);
        }
    } else {
        fprintf(stderr, "%d Number or identifier expected\n", yylineno);
    }
}
