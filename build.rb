#!/usr/bin/ruby

$BASENAME='ordo'

#============================

if ARGV[0] == 'gcc'

	puts 'gcc selected'
	$ADDPATH=''
	$CC='gcc'
	$NAME=$BASENAME
	$SWITCHES='-DNDEBUG -flto'

elsif ARGV[0] == 'debug'

	puts 'gcc debug'
	$ADDPATH=''
	$CC='gcc'
	$NAME=$BASENAME
	$SWITCHES=''

elsif ARGV[0] == 'profile'

	puts 'gcc profile'
	$ADDPATH=''
	$CC='gcc'
	$NAME=$BASENAME
	$SWITCHES='-DNDEBUG -pg -fno-inline'

elsif ARGV[0] == 'gcc64'

	puts 'gcc64 selected'
	$ADDPATH=''
	$CC='gcc'
	$NAME=$BASENAME+'-linux64'
	$SWITCHES='-DNDEBUG -flto'

elsif ARGV[0] == 'gcc32'

	puts 'gcc32 selected'
	$ADDPATH=''
	$CC='gcc -m32'
	$NAME=$BASENAME+'-linux32'
	$SWITCHES='-DNDEBUG -flto'

elsif ARGV[0] == 'win32'  

	puts 'win32 selected'
	$ADDPATH='/home/miguel/mingw/w32/bin'
	$CC='i686-w64-mingw32-gcc'
	$NAME=$BASENAME+'-win32'
	$SWITCHES='-DNDEBUG -flto'

elsif ARGV[0] == 'win64' 

	puts 'win64 selected'
	$ADDPATH='/home/miguel/mingw/w64/bin'
	$CC='x86_64-w64-mingw32-gcc' 
	$NAME=$BASENAME+'-win64'
	$SWITCHES='-DNDEBUG -flto'

else 

	puts 'gcc selected by default'
	$ADDPATH=''
	$CC='gcc'
	$NAME=$BASENAME
	$SWITCHES='-DNDEBUG -flto'

end

#=====================

$INCLUDE='-I myopt'
$WARNINGS='-Wwrite-strings -Wconversion -Wshadow -Wparentheses -Wlogical-op -Wunused -Wmissing-prototypes -Wmissing-declarations -Wdeclaration-after-statement -W -Wall -Wextra'
$OPT='-O3'
$LIB='-lm'
$SRC='myopt/myopt.c mystr.c proginfo.c pgnget.c randfast.c gauss.c groups.c csv.c encount.c main.c'
$EXE='-o ' + $NAME

#=====================
fragments = []
fragments << $SWITCHES
fragments << $INCLUDE
fragments << $WARNINGS
fragments << $OPT
fragments << $EXE
fragments << $SRC
fragments << $LIB

line=$CC
fragments.each do |x|
	line += ' ' + x
end

if ($ADDPATH != '')
	ENV['PATH'] += ':' + $ADDPATH
end

system line

