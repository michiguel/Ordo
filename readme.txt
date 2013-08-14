                                   ORDO v0.7                             

    Program to calculate ratings of individual chess engines (or players)
                Copyright (c) 2013 Miguel A. Ballicora
-------------------------------------------------------------------------------

FILES
In this distribution, files provided are readme.txt (this file) and versions in 
Linux (32 and 64 bits), or windows (64 and 32 bits). You can rename the proper 
file for your system to "ordo" (Linux) or "ordo.exe" (Windows). As an input 
example, a publicly available file results.pgn" is included. This was taken 
from http://www.inwoba.de/individual.7z (thanks to IPON rating list, from Ingo 
Bauer). A batch file (ordo_example.bat) was kindly prepared by Adam Hair, and 
it is included in the Windows distribution. It is a great start for Windows 
users.

USAGE
The input is a PGN file (http://en.wikipedia.org/wiki/Portable_Game_Notation), 
and Ordo automatically calculates a ranking based on the results of that file. 
The output could be a .csv file (comma separated value file) and/or a text 
file. The .csv file is an interesting option because it can be opened/imported 
by spreadsheet programs.

A simple use could be

ordo -p results.pgn

which will take the results from file.pgn and output the text ranking on the 
screen. If you want the results saved in a file rating.txt, you can do

ordo -p results.pgn -o rating.txt

Without any extra indication, the average rating of all the individuals will be 
2300. If you want a different overall average, you can set it with the switch 
"-a". For instance to have and average of 2500, you can do.

ordo -a 2500 -p results.pgn -o rating.txt

or

ordo -a 2500 -p results.pgn -c rating.csv

if you want the results as .csv (comma separated value), which is a format that 
can be opened with a spreadsheet program.

or 

ordo -a 2500 -p results.pgn -o rating.txt -c rating.csv

if you want both.

Important switches are -w, which provides to ordo what is the rating advantage 
equivalent for having white pieces. The switch -W calculates this for you 
automatically. In addition, -A will set the rating reference to a 
given player.

For instance:

ordo -a 2800 -A "Deep Shredder 12" -W -p results.pgn -o rating.txt

Will calculate the ratings from results.pgn, save it in rating.txt, calculate 
the white advantage automatically, and set 2800 to the engine Deep Shredder 12.

GROUP CONNECTION
Sometimes, some data set present players or groups/pools of players that did 
not play enough games against the rest. With the -g switch, an analysis of how 
many groups are in this situation will be saved in a file.

SIMULATION
The swich -s instructs Ordo to perform simulations. The program will will 
virtually 'replay' the games, and the results will be based on the
probabilities given by the ratings. After the simulations have been run a 
standard deviation is calculated. The error matrix file generated provides the 
errors between each of the pairs of players. To run thses simulations, a 
minimum reasonable number would be -s 100. Please, take into account that each 
simulation takes the same amount of time as each rating calculation.

SWITCHES
The list of the switches provided are:

usage: ordo [-OPTION]
 -h          print this help
 -H          print just the switches
 -v          print version number and exit
 -L          display the license information
 -q          quiet mode (no screen progress updates)
 -a <avg>    set rating for the pool average
 -A <player> anchor: rating given by '-a' is fixed for <player>, if provided
 -w <value>  white advantage value (default=0.0)
 -W          white advantage, automatically adjusted
 -z <value>  scaling: set rating for winning expectancy of 76% (default=202)
 -T          display winning expectancy table
 -p <file>   input file in PGN format
 -c <file>   output file (comma separated value format)
 -o <file>   output file (text format), goes to the screen if not present
 -g <file>   output file with group connection info (no rating output on screen)
 -s  #       perform # simulations to calculate errors
 -e <file>   saves an error matrix, if -s was used
 -F <value>  confidence (%) to estimate error margins. Default is 95.0

MEMORY LIMITS
For now, the program can handle 3 million games, 50,000 players, and 
one megabyte of memory for name of the players.

ORDOPREP
A utility is included that will shrink the PGN file to "results only". In 
addition, it could discard players that won all games, or lost all games (which 
generally bring problems). Another switches allow to exclude players that do 
not have a minimum performance or played too few games.

A typical use will be

ordoprep -p raw.pgn -o shrunk.pgn

Which saves in shrunk.pgn a pgn file with only results. You can add switches 
like this

ordoprep -p raw.pgn -o shrunk.pgn -d -m 5 -g 20

'-d' tells ordoprep to discard players with 100% or 0% performance.
'-m 5' will exclude player who did not reach a 5% performance.
'-g 20' will exclude players with less than 20 games.

After all this, 'shrunk.pgn' could be used as input for 'ordo'

ACKNOWLEDGMENTS
Adam Hair has been testing Ordo extensively, and suggested valuables ideas that 
certainly improved the program.
-------------------------------------------------------------------------------

ordo v0.7
Copyright (c) 2013 Miguel A. Ballicora

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
