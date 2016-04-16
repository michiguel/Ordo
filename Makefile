CC = gcc
CFLAGS = -DNDEBUG -DMY_SEMAPHORES -flto -I myopt -I sysport
CFLAGSD = -g -DMY_SEMAPHORES -I myopt -I sysport
WARN = -Wwrite-strings -Wconversion -Wshadow -Wparentheses -Wlogical-op -Wunused -Wmissing-prototypes -Wmissing-declarations -Wdeclaration-after-statement -W -Wall -Wextra
OPT = -O3
LIBFLAGS = -lm -lpthread

EXE = ordo

SRC = myopt/myopt.c sysport/sysport.c mystr.c proginfo.c pgnget.c randfast.c gauss.c groups.c cegt.c indiv.c encount.c ratingb.c rating.c xpect.c csv.c fit1d.c mymem.c relprior.c report.c relpman.c plyrs.c namehash.c inidone.c rtngcalc.c ra.c sim.c summations.c bitarray.c strlist.c justify.c myhelp.c mytimer.c main.c
DEPS = myopt/myopt.h sysport/sysport.h boolean.h  datatype.h  gauss.h  groups.h  mystr.h  mytypes.h  ordolim.h  pgnget.h  proginfo.h  progname.h  randfast.h  version.h cegt.h indiv.h encount.h xpect.h csv.h ratingb.h fit1d.h rating.h report.h relprior.h relpman.h mymem.h namehash.h inidone.h rtngcalc.h ra.h sim.h summations.h bitarray.h strlist.h plyrs.h justify.h mytimer.h myhelp.h
OBJ = myopt/myopt.o sysport/sysport.o mystr.o proginfo.o pgnget.o randfast.o gauss.o groups.o cegt.o indiv.o encount.o ratingb.o rating.o xpect.o csv.o fit1d.o mymem.o report.o relprior.o relpman.o plyrs.o namehash.o inidone.o rtngcalc.o ra.o sim.o summations.o bitarray.o strlist.o justify.o myhelp.o mytimer.o main.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ordo: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(WARN) $(OPT) $(LIBFLAGS)

all:
	$(CC) $(CFLAGS) $(WARN) $(OPT) -o $(EXE) $(SRC) $(LIBFLAGS)

debug:
	$(CC) $(CFLAGSD) $(WARN) $(OPT) -o $(EXE) $(SRC) $(LIBFLAGS)

install:
	cp $(EXE) /usr/local/bin/$(EXE)

clean:
	rm -f *.o *~ myopt/*.o ordo-v*.tar.gz ordo-v*-win.zip *.out









