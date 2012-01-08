#if !defined(H_MYOPT)
#define H_MYOPT
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

enum char_options {END_OF_OPTIONS = -1};

extern int 		opt_index;      
extern char *	opt_arg;    

extern int 		options(int argc, char *argv[], const char *legal);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif

