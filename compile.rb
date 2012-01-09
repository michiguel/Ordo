#!/usr/bin/ruby

if ARGV[0] == 'gcc'

	$NEWPATH=''
	$CC='gcc'
	$NAME='ordo'

elsif ARGV[0] == 'win32'  

	$NEWPATH='/home/miguel/install/mingw-w32-bin_x86_64-linux_20111220/bin'
	$CC='i686-w64-mingw32-gcc'
	$NAME='ordo-win32'

elsif ARGV[0] == 'win64' 

	$NEWPATH='/home/miguel/mingw-w64/bin'
	$CC='x86_64-w64-mingw32-gcc' 
	$NAME='ordo-win64'

else 

	$NEWPATH=''
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
$EXE='-o ordo'

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

system 'PATH=$PATH:' + $NEWPATH
system line

