#!/usr/bin/ruby

def multichomp x
	y = x
	z = x.chomp
	while z != y do
		y = z
		z = z.chomp
	end
	z
end

#
# Default version from previous one
#
current_version=`cat version.txt`
default = multichomp(current_version)
puts default

# Edit version to version.txt
system 'zenity --entry --text "Version" --entry-text "' + default + '" > zenityanswer.txt'
version=`cat zenityanswer.txt`
version = multichomp(version)
system 'echo "' + version + '" > version.txt'

# update
version_h = '#define VERSION "' + version + '"' + "\n\n"


# edit commit message
title    = 'Commit Message'
file_inp = 'commit-inp.txt'
file_out = 'commit-out.txt'

File.open file_inp, 'w' do |f|
	f.write "[" + version + "]\n\n"
	f.write "*"
end

editmsg_string = 'zenity --text-info --title='+ title +' --filename='+ file_inp +' --editable > ' + file_out
system editmsg_string


# commit actions

File.open "version.h", 'w' do |f|
	f.write version_h
	f.write ""
end
system "git tag -f " + version
system "git commit -a --file=" + file_out

# clean up
system 'rm ' + file_inp
system 'rm ' + file_out
system 'rm ' + 'zenityanswer.txt'


