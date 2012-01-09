#!/usr/bin/ruby

if ARGV[0] == 'gcc'

	puts 'gcc selected'
	$ADDPATH=''
	$CC='gcc'
	$NAME='ordo'

elsif ARGV[0] == 'win32'  

	puts 'win32 selected'
	$ADDPATH='/home/miguel/mingw/w32/bin'
	$CC='i686-w64-mingw32-gcc'
	$NAME='ordo-win32'

elsif ARGV[0] == 'win64' 

	puts 'win64 selected'
	$ADDPATH='/home/miguel/mingw/w64/bin'
	$CC='x86_64-w64-mingw32-gcc' 
	$NAME='ordo-win64'

else 

	$ADDPATH=''
	$CC=''
	$NAME=''
	puts 'no parameter given'
	exit
end

#=====================

$SWITCHES='-DNDEBUG'
$INCLUDE='-I myopt'
$WARNINGS='-Wwrite-strings -Wconversion -Wshadow -Wparentheses -Wlogical-op -Wunused -Wmissing-prototypes -Wmissing-declarations -Wdeclaration-after-statement -W -Wall -Wextra'
$OPT='-O2'
$LIB='-lm'
$SRC='myopt/myopt.c mystr.c proginfo.c main.c'
$EXE='-o ' + $NAME

#=====================
fragments = []
fragments << $SWITCHES
fragments << $INCLUDE
fragments << $WARNINGS
fragments << $OPT
fragments << $LIB
fragments << $EXE
fragments << $SRC


line=$CC
fragments.each do |x|
	line += ' ' + x
end

oldpath = ENV['PATH']
ENV['PATH'] = oldpath + ':' + $ADDPATH

system line

