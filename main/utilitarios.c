/** @file utilitarios.c - Diversos utilitários para lidar com strings, etc.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

// A documentação de cada função se encontra no arquivo header.

bool str_startswith(const char *str, const char *substr) {
  while (*substr != '\0') {
    if (*str != *substr) {
      return false;
    }
    str++;
    substr++;
  }
  return true;
}


void urldecode2(char *dst, const char *src) {
    char a, b;
    while(*src) {
        if((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if(a >= 'a')
                a -= 'a'-'A';
            if(a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if(b >= 'a')
                b -= 'a'-'A';
            if(b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if(*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}


