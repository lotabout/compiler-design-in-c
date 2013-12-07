/* nfa.c -- Make a NFA from a LeX input file using Thompson's construction */

#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* not yet have these headers below */
#include "nfa.h"
#include "globals.h"


#ifdef DEBUG
    int Lev = 0;
    #define ENTER(f) printf("%*senter %s [%c][%1.10s] \n", \
                            Lev++ * 4, "", f, Lexeme, Input);
    #define LEAVE(f) printf("%*sleave %s [%c][%1.10s]\n", \
                            --Lev * 4, "", f, Lexeme, Input);
#else
    #define ENTER(f)
    #define LEAVE(f)
#endif

/*-----------------------------------------------------------------------------
 * Error processing stuff. Not that all errors are fatal.
 *---------------------------------------------------------------------------*/
typedef enum {
    E_MEM,     /* Not enough memeory for NFA" */
    E_BADEXPR, /* Malformed regular expression" */
    E_PAREN,   /* Missing close parenthesis" */
    E_STACK,   /* Internal error: Discard stack full" */
    E_LENGTH,  /* Too many regular expressions or expression too long" */
    E_BRACKET, /* Missing [ in character class" */
    E_BOL,     /* ^ must be at start of expression of after [" */
    E_CLOSE,   /* + ? or * must follow an expression or subexpression" */
    E_STRINGS, /* Too many characters in accept actions" */
    E_NEWLINE, /* Newline in quoted string, use \\n to get new line into
                  expression" */
    E_BADMAC,  /* Missing } in macro expansion" */
    E_NOMAC,   /* Macro doesn't exist" */
    E_MACDEPTH,/* Macro expansion nested too deeply" */
} ERR_NUM;

static char *Errmsgs[] = /* Indexed by ERR_NUM */
{
    "Not enough memeory for NFA",
    "Malformed regular expression",
    "Missing close parenthesis",
    "Internal error: Discard stack full",
    "Too many regular expressions or expression too long",
    "Missing [ in character class",
    "^ must be at start of expression of after [",
    "+ ? or * must follow an expression or subexpression",
    "Too many characters in accept actions",
    "Newline in quoted string, use \\n to get new line into expression",
    "Missing } in macro expansion",
    "Macro doesn't exist",
    "Macro expansion nested too deeply",
};

static void parse_err(ERR_NUM type)
{
    fprintf(stderr, "ERROR (line %d) %s\n%s\n", Actual_lineno,
            Errmsgs[(int)type], S_input);
    while (++S_input <= Input) {
        putc('_', stderr);
    }

    fprintf(stderr, "^\n");
    exit(1);
}
