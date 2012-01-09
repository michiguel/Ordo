#!/bin/sh

CC=gcc

$CC -DNDEBUG -I myopt -Wall -Wextra -lm myopt/myopt.c mystr.c proginfo.c main.c -o ordo




