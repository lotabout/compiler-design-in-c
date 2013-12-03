#include "lex.h"
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

char *yytext = "";   /* lexeme (not '\0' terminated.) */
int yyleng   = 0;    /* lexeme length                 */
int yylineno = 0;    /* input line number             */

token_t lex(void)
{
    static char input_buffer[128];
    char *current;

    current = yytext + yyleng;  /* skip current lexeme */

    while (true) {
        while (*current == '\0') {
            /* Get new lines, skipping any leading white space on the line until a
             * nonblank line is found. 
             */ 
            current = input_buffer;
            if (!gets(input_buffer)) {
                *current = '\0';
                return EOI;
            }
            ++yylineno;
            while (isspace(*current)) {
                ++current;
            }
        }

        for (; *current; ++current) {
            /* Get the next token */

            yytext = current;
            yyleng = 1;

            switch (*current) {
                case EOF: 
                    return EOI;
                    break;
                case ';':
                    return SEMI;
                    break;
                case '+':
                    return PLUS;
                    break;
                case '*':
                    return TIMES;
                    break;
                case '(':
                    return LP;
                    break;
                case ')':
                    return RP;
                    break;

                case '\n':
                case '\t':
                case ' ':
                    break;

                default:
                    if (! isalnum(*current)) {
                        fprintf(stderr, "Ignoring illegal input <%c>\n",
                                *current);
                    } else {
                        while (isalnum(*current)) {
                            ++current;
                        }
                        yyleng = current - yytext;
                        return NUM_OR_ID;
                    }
                    break;
            } /* end of switch */
        } /* end of for */
    } /* end of while */
}

static token_t Lookahead = UNKNOWN;    /* look ahead token */

bool match(token_t token)
{
    /* Return true if "token" matches the current lookahead symbol */

    if (Lookahead == UNKNOWN) {
        Lookahead = lex();
    }

    return token == Lookahead;
}

void advance()
{
    /* Advance the lookahead to the next input symbol. */
    Lookahead = lex();
}
