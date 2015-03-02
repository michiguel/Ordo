# ORDO 
Ratings for chess and other games 

Copyright 2015 Miguel A. Ballicora

Ordo is a program designed to calculate ratings of individual chess engines (or players).
It has a similar concept than the [Elo rating](http://en.wikipedia.org/wiki/Elo_rating_system), but with a different model and algorithm. 
Ordo keeps consistency among ratings because it calculates them considering all results at once.
The source code is available under the GPL.

### Compilation
Program can be compile and installed in GNU/Linux with

`make`

`make install`

### Usage
The input should be a file that adheres to the [PGN standard](http://en.wikipedia.org/wiki/Portable_Game_Notation). 
Based on the results in that file, Ordo automatically calculates a ranking . 
The output can be a plain text file and/or a comma separated value file.
The `.csv` file is an interesting option because it can be opened/imported by most spreadsheet programs. 
Once imported, the user can choose to format the output externally.
The simplest way to use Ordo is typing in the command line:

`ordo -p games.pgn`

which will take the results from games.pgn and output the text ranking on the screen. 
If you want to save the results in a file ratings.txt, you can run:

`ordo -p games.pgn -o ratings.txt`

By default, the average rating of all the individuals is 2300. 
If you want a different overall average, you can use the switch `-a` to set it. 
For instance to have and average of 2500, you can do:
	
`ordo -a 2500 -p games.pgn -o ratings.txt`

or if you want the results in .csv format, use the switch -c.

`ordo -a 2500 -p games.pgn -c rating.csv`

If you want both, you can use:

`ordo -a 2500 -p games.pgn -o ratings.txt -c rating.csv`

In addition, -A will fix the rating of a given player as a reference anchor for the whole pool of players.

`ordo -a 2800 -A "Engine X" -p games.pgn -o ratings.txt`

That will calculate the ratings from games.pgn, save it in ratings.txt, and anchor the engine Engine X to a rating of 2800.
Names that contain spaces should be surrounded by quote marks as in this example.

### Help
Other switches are available and information about them can be obtained by typing

`ordo -h`

That will list the help. For more information and a [manual](https://docs.google.com/viewer?a=v&pid=sites&srcid=ZGVmYXVsdGRvbWFpbnxnYXZpb3RhY2hlc3NlbmdpbmV8Z3g6M2M0YjhlYzRiOGE2NDRlMA), 
please go to [Ordo-info](https://sites.google.com/site/gaviotachessengine/ordo/ordo-readme).

### Acknowledgments
Adam Hair has extensively tested and suggested valuable ideas.


