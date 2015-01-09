#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "tools.h"

/* print error message and exit */
void ferr(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    exit(1);
}
