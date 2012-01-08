/*************************************************************************************************************/
#include "myopt.h"

const char *license_str =
"Copyright (c) 2012 Miguel A. Ballicora\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES\n"
"OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
"NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT\n"
"HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
"WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
"OTHER DEALINGS IN THE SOFTWARE."
;

static void parameter_error(void);
static void example (void);
static void usage (void);

/* VARIABLES */

	char *	inputname;
	char *	folder = "";
	char *	folders;
	int		sep;

	static bool_t VERIFICATION_MODE, QUIET_MODE;
	static int TBPIECES;

	static const char *copyright_str = 
		"Copyright (c) 2012 Miguel A. Ballicora\n"
		"There is NO WARRANTY of any kind\n"
		;

	static const char *intro_str =
		"Program to rate individual players.\n"
		;

	static const char *example_str =
		" Example.\n"
		;

	static const char *help_str =
		" -h        print this help\n"
		" -v        print version number and exit\n"
		" -L        display the license information\n"
		" -q        quiet (no output except error messages)\n"
		" -i <file> input file\n"
		"\n"
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		;

const char *OPTION_LIST = "vhi:qL";

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/

int main (int argc, char *argv[])
{
	char *f;
	int individual_mode, version_mode, help_mode, license_mode;

	/* defaults */
	version_mode = FALSE;
	license_mode = FALSE;
	help_mode = FALSE;
	input_mode = FALSE;
	f = NULL;
	QUIET_MODE = FALSE;

	while (END_OF_OPTIONS != (op = options (argc, argv, OPTION_LIST))) {
		switch (op) {
			case 'v':	version_mode = TRUE; 	break;
			case 'L':	version_mode = TRUE; 	
						license_mode = TRUE;
						break;
			case 'h':	help_mode = TRUE;		break;
			case 'i': 	input_mode = TRUE;
					 	f = opt_arg;
						break;
			case 'q':	QUIET_MODE = TRUE;	break;
			case '?': 	parameter_error();
						exit(EXIT_FAILURE);
						break;
			default:	fprintf (stderr, "ERROR: %d\n", op);
						exit(EXIT_FAILURE);
						break;
		}		
	}

	/*----------------------------------*\
	|	Return version
	\*----------------------------------*/
	#include "version.h"
	#include "progname.h"
	if (version_mode) {
		printf ("%s %s\n",PROGRAM_NAME,VERSION);
		if (license_mode)
 			printf ("%s\n", license_str);
		return EXIT_SUCCESS;
	}
	if (argc < 2) {
		fprintf (stderr, "%s %s\n",PROGRAM_NAME,VERSION);
		fprintf (stderr, "%s", copyright_str);
		fprintf (stderr, "for help type:\n%s -h\n\n", PROGRAM_NAME);		
		exit (EXIT_FAILURE);
	}
	if (help_mode) {
		printf ("\n%s", intro_str);
		example();
		usage();
		printf ("%s\n", copyright_str);
		exit (EXIT_SUCCESS);
	}
	if ((argc - opt_index) > 1) {
		/* too many parameters */
		fprintf (stderr, "Extra parameters parameters present\n");
		exit(EXIT_FAILURE);
	}
	if (input_mode && argc != opt_index) {
		fprintf (stderr, "Extra parameters parameters present\n");
		exit(EXIT_FAILURE);
	}
	if (!input_mode && argc == opt_index) {
		fprintf (stderr, "Need file name to proceed\n");
		exit(EXIT_FAILURE);
	}
	/* get folder, should be only one at this point */
	while (opt_index < argc ) {
		inputf = argv[opt_index++];
	}

}


/*--------------------------------------------------------------*\
|
|	END OF MAIN
|
\**/


static void parameter_error(void) {	printf ("Error in parameters\n"); return;}

static void
example (void)
{
	fprintf (stderr, "\n"
		"quick example: %s gtb\n"
		"%s"
		, PROGRAM_NAME
		, example_str);
	return;
}

static void
usage (void)
{
	fprintf (stderr, "\n"
		"usage: %s [-OPTION]... [folder]\n"
		"%s"
		, PROGRAM_NAME
		, help_str);
}
