#!/bin/sh

gcc -DNDEBUG -I myopt -Wall -Wextra myopt/myopt.c mystr.c proginfo.c main.c -lm -o ordo

