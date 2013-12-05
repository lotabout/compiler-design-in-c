/* global.h
 *
 * Global variable definition 
 * TODO: Look for a way to remove these macros. */
#ifdef ALLOC
    #define CLASS
    #define I(x) x
#else
    #define CLASS extern
    #define I(x)
#endif

#define MAXINP = 2048;    /* Maximum rule size */

CLASS int Verbose I( = 0 ); /* Print statistics */
CLASS int No_lines I( = 0); /* Supress #line directive. */
CLASS int Unix  I( = 0 ); /* Use UNIX-style newlines */
CLASS int Public I( = 0); /* make static symbols public */
CLASS char *Templage I( = "lex.par"); /* State-machine driver template */
CLASS int Actual_lineno I( = 1); /* Line number of first line of a
                                    multiple-line rule */
CLASS char Input_buf[MAXINP];   /* Line buffer for input */
CLASS char *Input_file_name;    /* Input file name (for #line) */
CLASS FILE *Ifile;  /* Input stream */
CLASS FILE *Ofile;  /* Output stream */
#undef CLASS
#undef I
