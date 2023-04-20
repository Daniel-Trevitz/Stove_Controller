// https://raw.githubusercontent.com/swarajsatvaya/UrlDecoder-C/master/urlDecoder.c
#include "urldecode.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *urlDecode(const char *str)
{
    //int d = 0; /* whether or not the string is decoded */

    char *dStr = (char *) malloc(strlen(str) + 1);
    char eStr[] = "00"; /* for a hex code */

    strcpy(dStr, str);

    //while(!d) {
    //d = 1;
    int i; /* the counter for the string */

    for(i=0; i<strlen(dStr); ++i)
    {
        if(dStr[i] == '%')
        {
            if(dStr[i+1] == 0)
                return dStr;

            char c = dStr[i+1];
            char d = dStr[i+2];
            if(isxdigit(c) && isxdigit(d))
            {
                //d = 0;

                /* combine the next to numbers into one */
                eStr[0] = dStr[i+1];
                eStr[1] = dStr[i+2];

                /* convert it to decimal */
                long int x = strtol(eStr, NULL, 16);

                /* remove the hex */
                memmove(&dStr[i+1], &dStr[i+3], strlen(&dStr[i+3])+1);

                dStr[i] = x;
            }
        }
        else if(dStr[i] == '+')
        {
            dStr[i] = ' ';
        }
    }
    //}

    return dStr;
}

