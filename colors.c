#include <stdio.h>
#include "colors.h"

void black()
{
    printf("\033[30m");
}

void red()
{
    printf("\033[0;31m");
}

void green()
{
    printf("\033[0;32m");
}

void yellow()
{
    printf("\033[0;33m");
}

void blue()
{
    printf("\033[0;34m");
}

void magenta()
{
    printf("\033[0;35m");
}

void cyan()
{
    printf("\033[0;36m");
}

void white()
{
    printf("\033[0;37m");
}

void reset()
{
    printf("\033[0m");
}

void bold()
{
    printf("\033[1m");
}