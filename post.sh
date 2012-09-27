#!/bin/sh

FILENAME=ordowin.zip
USER=mballicora
HOST=ftp.rcn.com
PASSWORD=`zenity --title='FTP Password' --hide-text --text='Enter password to ftp.rcn.com' --entry`


./build.rb win32
./build.rb win64
rm -f $FILENAME
zip $FILENAME ordo-win32 ordo-win64

echo Logging to site...
ftp -n $HOST << EOF
user $USER $PASSWORD
binary
delete $FILENAME
put $FILENAME
quit
EOF
echo Done.
