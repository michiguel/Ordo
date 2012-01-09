#!/bin/sh


#PATH=$PATH:/home/miguel/mingw-w64/bin

CC=gcc

#CC=x86_64-w64-mingw32-gcc

$CC -DNDEBUG -I myopt -Wall -Wextra myopt/myopt.c mystr.c proginfo.c main.c -lm -o ordo




