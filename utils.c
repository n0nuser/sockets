#include <stdio.h>
#include "utils.h"

void debug(int debug, char texto[])
{
        if (debug)
                printf("%s",texto);
}