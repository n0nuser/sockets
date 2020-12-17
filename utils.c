#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

void printChars(char buf[])
{
    setlocale(LC_ALL, "C");
    int i = 0;
    while (i < strlen(buf))
    {
        unsigned int c = (unsigned int)(unsigned char)buf[i]; // (2)

        if (isprint(c) && c != '\\')
            putchar(c);
        else
            printf("\\x%02x", c);
        i++;
    }
}