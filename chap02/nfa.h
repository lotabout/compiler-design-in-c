/* nfa.h 
 *
 */

/* Data structures and macros */

typedef struct _nfa_state{
    int edge;   /* Label for edge: character, CCL, EMPTY or EPSILON */
    SET *bitset;    /* set to store character classes */
    struct _nfa_state *next;    /* next state */
    struct _nfa_state *next2;   /* another next state if edge == EPSILON */
    char *accept;   /* NULL if not an accepting state, else a pointer to the
                       action string */
    int anchor; /* Says whether pattern is anchored and, if so where */
} nfa_state;

typedef enum {
    EPSILON = -1,   /* Non-character values of NFA.edge */
    CCL     = -2,
    EMPTY   = -3,
} edge_type;

/* values of the anchor field: */
typedef enum {
    NONE  = 0,               /* Not anchored */
    START = 1,               /* Anchored at start of line */
    END   = 2,               /* Anchored at end of line */
    BOTH  = (START | END),   /* Anchored in both places */
} anchor_type;

/* Other Definitions and Prototypes */
const int NFA_MAX = 768;    /* Maximum number of NFA states in a single
                               machine.  NFA_MAX * sizeof(NFA) cannot exceed
                               64K. */
const int STR_MAX = (10 * 1024);    /* Total space that can be used by the
                                       accept strings. */

/* these three are in nfa.c */
void new_macro(char *definition);
void print_macros(void);
nfa_state *thompson(char *(*input_func)(), int *max_state, 
                    nfa_state **start_state);

/* in printnfa.c */
void print_nfa(nfa_state *nfa, int len, nfa_state *start);
