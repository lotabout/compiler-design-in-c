/* nfa.c -- Make a NFA from a LeX input file using Thompson's construction */

#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* not yet have these headers below */
#include "tools/debug.h"
#include "tools/set.h"
#include "tools/hash.h"
#include "compiler.h"
#include "stack.h"


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

/*-----------------------------------------------------------------------------
 * Memory management -- states and String
 *
 * 1. malloc a serious memory: Nfa_states.
 * 2. Use a stack to manually save "freed" memory.
 * 3. when receiving allocation request, first check if the stack is not
 * empty, if not, that means we can re-use the memory it saves. Otherwise get
 * our memory from "Nfa_states".
 *---------------------------------------------------------------------------*/
static nfa_state *Nfa_states;   /* state-machine array */
static int Nstates = 0;         /* # of NFA states in machine */
static int Next_alloc;          /* Index of next element of the array */

#define SSIZE 32

static nfa_state *Sstack[SSIZE];    /* Stack used by new() */
static nfa_state **Sp = &Sstack[-1];    /* Stack pointer, i.e &(Sstack - 1) */

#define STACK_OK    (INBOUNDS(Sstack, Sp)) /* true if stack not full or empty
                                            */
#define STACK_USED()    ((Sp-Sstack) + 1)   /* slots used */
#define CLEAR_STACK()   (Sp = Sstack - 1)   /* reset the stack */
#define PUSH(x)         (*++Sp = (x))       /* put x on stack */
#define POP()           (*Sp --)            /* get x from stack */

static int *Strings;    /* Place to save accepting strings */
static int *Savep;      /* Current position in String array. */

static nfa_state *new()
{
    nfa_state *p;
    static int first_time = 1;
    if (first_time) {
        Nfa_states = (nfa_state *) calloc(NFA_MAX, sizeof(nfa_state));
        if (Nfa_states == NULL) {
            parse_err(E_MEM);
        }
        first_time = 0;
        Sp = &Sstack[-1];
    }

    if (++Nstates >= NFA_MAX) {
        parse_err(E_LENGTH);
    }

    /* if the stack is not OK, it's empty */
    p = !STACK_OK() ? &Nfa_states[Next_alloc++] : POP();
    p->edge = EPSILON;

    return p;
}

static void discard(nfa_state *nfa_to_discard)
{
    --Nstates;

    memset(nfa_to_discard, 0, sizeof(*nfa_to_discard));
    nfa_to_discard->edge = EMPTY;
    PUSH(nfa_to_discard);
    if (! STACK_OK()) {
        parse_err(E_STACK);
    }
}

/* string management function */
static char *save(char *str)
{
    char *textp, *startp;
    int len;
    static int first_time = 1;

    if (first_time) {
        Savep = Strings = (int *)malloc(STR_MAX);
        if (Savep == NULL) {
            parse_err(E_MEM);
        }
        first_time = 0;
    }

    if (*str == '|') {
        return (char*)(Savep + 1);
    }

    *Savep++ = Lineno;

    for (textp = (char *)Savep; *str; *textp++=*str++) {
        if (textp >= (char *)(Strings + (STR_MAX -1))) {
            parse_err(E_STRINGS);
        }
    }

    *textp++ = '\0';

    /* Increment Savep past the text. "len" is initialized to the string
     * length. The "len/sizeof(int)" truncates the size down to an even
     * multiple of the current int size. The "+(len % sizeof(int) != 0) adds 1
     * to the truncated size if the string length isn't an even multiple of
     * the int size (the != operator evaluates to 1 or 0). Return a pointer to
     * the string itself. */
    startp = (char *)Savep;
    len = textp - startp;
    Savep += (len/sizeof(int)) + (len % sizeof(int) != 0);
    return startp;
}

/*-----------------------------------------------------------------------------
 * macro support
 *---------------------------------------------------------------------------*/
#define MAC_NAME_MAX 34     /* maximum name length */
#define MAC_TEXT_MAX 80     /* maximum amount of expansion text */

typedef struct _MACRO {
    char name[MAC_NAME_MAX];
    char text[MAC_TEXT_MAX];
} MACRO;

static HASH_TAB *Macros;    /* symbol table for macro definition */

void new_macro(char *def)
{
    /* Add a new macro to the table, If two macros have the same name, the
     * second one takes precedence. A definition takes the form:
     *      name <whitespace> text [<whitespace>]
     * whitespace at the end of the line is ignored.
     */

    /* used for hash function */
    unsigned hash_add(void);    /* TODO: figure out its usage */

    char *name;     /* name component of macro definition */
    char *text;     /* text part of macro definition */
    char *edef;     /* pointer to end of text part */
    MACRO *p;
    static int first_time = 1;

    if (first_time) {
        first_time = 0;
        Macros = maketab(31, hash_add, strcmp);
    }

    for (name = def; *def && !isspace(*def); def++) {
        /* Isolate name */
    }

    if (*def) {
        *def++ = '\0';
    }

    /* Isolate the definition text. This process is complicated because you
     * need to discard any trailing whitespace on the line. The first while
     * loop skips the preceding whitespace. The for loop is looking for end of
     * string. If you find a white character (and the \n at the end of string
     * is whitespace), remember the position as a potential end of string */
    while (isspace(*def)) {
        /* skip up to macro body */
        ++def;
    }

    text = def;     /* remember start of replacement text */
    edef = NULL;    /* strip trailing white space */
    while (*def) {
        if (!isspace(*def)) {
            ++def;
        } else {
            for (edef = def++; isspace(*def); ++def) {
                /* pass */
            }
        }
    }

    if (edef) {
        *def = '\0';   
    }

    /* add the macro to the symbol table */
    p = (MACRO *) newsym(sizeof(MACRO));
    strncpy(p->name, name, MAC_NAME_MAX);
    strncpy(p->text, text, MAC_TEXT_MAX);
    addsym(Macros, p);
}

static char *expand_macro(char **namep)
{
    /* Return a pointer to the contents of a macro having the indicated name.
     * Abort with a message if no macro exits. The macro name includes the
     * brackets, which are destroyed by the expansion process. *namep is
     * modified to point past the close brace. 
     */

    char *p = NULL;
    MACRO *mac = NULL;

    p = strchr(++(*namep), '}'); /* skip { and find } */
    if (p == NULL) {
        parse_err(E_BADMAC);
    } else {
        *p++ = '\0';    /* Overwrite close brace */

        mac = (MACRO *) findsym(Macros, *namep);
        if (mac == NULL) {
            parse_err(E_NOMAC);
        }

        *namep = p;
        return mac->text;
    }
    return "ERROR";     /* If you get here, it's a bug */
}

static print_a_macro(MACRO *mac)
{
    /* Workhorse function function needed by ptab() call in printmacs(), below
     */
    print("%-16s--[%s]--\n", mac->name, mac->text);
}

/* print all macros to stdout */
void printmacs()
{
    if (!Macros) {
        printf("\tThere are no macros\n");
    } else {
        printf("\nMACROS:\n");
        ptab(Macros, print_a_macro, NULL, 1);
    }
}

/*-----------------------------------------------------------------------------
 * LeX's own lexical analyzer
 *---------------------------------------------------------------------------*/
typedef enum {
    EOS = 1,      /* end of string     */
    ANY,          /* .                 */
    AT_BOL,       /* ^                 */
    AT_EOL,       /* $                 */
    CCL_END,      /* ]                 */
    CCL_START,    /* [                 */
    CLOSE_CURLY,  /* }                 */
    CLOSE_PAREN,  /* )                 */
    CLOSURE,      /* *                 */
    DASH,         /* -                 */
    END_OF_INPUT, /* EOF               */
    L,            /* literal character */
    OPEN_CURLY,   /* {                 */
    OPEN_PAREN,   /* (                 */
    OPTIONAL,     /* ?                 */
    OR,           /* |                 */
    PLUS_CLOSE,   /* +                 */
} TOKEN;

TOKEN Tokmap[] = 
{
/*  ^@  ^A  ^B  ^C  ^D  ^E  ^F  ^G  ^H  ^I  ^J  ^K  ^L  ^M  ^N   */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L, 

/*  ^O  ^P  ^Q  ^R  ^S  ^T  ^U  ^V  ^W  ^X  ^Y  ^Z  ^[  ^\  ^]   */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L, 

/*  ^^  ^_  SPACE   !   "   #   $       %   &   , */
    L,  L,  L,      L,  L,  L,  AT_EOL, L,  L,  L,

/*  (           )            *        +           ,  -     .    */
    OPEN_PAREN, CLOSE_PAREN, CLOSURE, PLUS_CLOSE, L, DASH, ANY,

/*  /   0   1   2   3   4   5   6   7   8   9   :   ;   <   =   */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L, 

/*  >   ?                                                       */
    L,  OPTIONAL,

/*  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L, 

/*  O   P   Q   R   S   T   U   V   W   X   Y   Z               */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,

/*  [          \  ]        ^                                    */
    CCL_START, L, CCL_END, AT_BOL,

/*  _   `   a   b   c   d   e   f   g   h   i   j   k   l   m   */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L, 

/*  n   o   p   q   r   s   t   u   v   w   x   y   z           */
    L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,

/*  {           |   }            DEL                            */
    OPEN_CURLY, OR, CLOSE_CURLY, L,
};

static char *(*Ifunc)();    /* Input function pointer */
static char *Input = "";    /* current position in input string */
static char *S_input;       /* Beginning of input string */
static TOKEN Current_tok;   /* Current token */
static int Lexeme;          /* Value associated with LITERAL */

#define MATCH(t) (Current_tok == (t))

/*-----------------------------------------------------------------------------
 * Lexical analyzer:
 *
 * Lexical analysis is trivial because all lexemes are single-character
 * values. The only complications are escape sequences and quoted strings,
 * both of which are handled by advance(), below. This routine advances past
 * the current token, putting the new token into Current_tok and the
 * equivalent lexeme into Lexeme. If the character was escaped, Lexeme holds
 * the actual value. For example, if a '\s' is encountered, Lexeme will hold a
 * space character. The MATCH(x) macro returns true if x matches the current
 * token. Advance both modifiers Current_tok to the current token and return
 * it. 
 */
static int advance(void)
{
    static int inquote = 0;    /* Processing quoted string */
    int saw_esc;               /* saw a backslash '\'      */
    static char *stack[SSIZE]; /* input-source stack       */
    static char **sp = NULL;   /* stack pointer            */

    if (sp == NULL) {
        /* initialize sp. Necessary for large model */
        sp = stack - 1;
    }

    /* Get another line */
    if (Current_tok == EOS) {
        if (inquote) {
            parse_err(E_NEWLINE);
        }

        do {
            Input = Ifunc();
            if (Input == NULL) {
                Current_tok = END_OF_INPUT;
                goto exit;
            }

            /* ignoring leading space... and blank lines */
            while (isspace(*Input)) {
                Input ++;
            }
        } while ((*Input) == '\0');

        S_input = Input;    /* Remember start of line for error messages. */
    }

    while (*Input == '\0') {
        /* Restore previous input source */
        if (INBOUNDS(stack, sp)) {
            INput = *sp--;
            continue;
        }

        Current_tok = EOS;  /* No more input sources to restore */
        Lexeme = '\0';      /* i.e. you're at the real end of string */
        goto exit;
    }

    if (!inquote) {
        while (*Input == '{') {
            /* Macro expansion required 
             * Stack current input string adn replace it with the macro body */
            *++sp = Input;
            Input = expand_macro(sp);

            if (TOOHIGH(stack, sp)) {
                parse_err(E_MACDEPTH);  /* stack overflow */
            }
        }
    }

    /* At either start and end of a quoted string. All characters are treated
     * as literals while inquote is true */
    if (*Input == '"') {
        inquote = ~inquote;
        if (! *++Input) {
            Current_tok = EOS;
            Lexeme = '\0';
            goto exit;
        }
    }

    saw_esc = (*Input == '\\');

    if (!inquote) {
        if (isspace(*Input)) {
            Current_tok = EOS;
            Lexeme = '\0';
            goto exit;
        }
        Lexeme = esc(&input);
    } else {
        if (saw_esc && Input[1] = '"') {
            Input += 2;
            Lexeme = '"';
        } else {
            Lexeme = *Input ++;
        }
    }

    Current_tok = (inquote || saw_esc) ? L : Tokmap[Lexeme];

exit:
    return Current_tok;
}
