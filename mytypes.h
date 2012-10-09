#if !defined(H_MYTYP)
#define H_MYTYP
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#if defined(MINGW)
	#include <windows.h>
	#if !defined(MVSC)
		#define MVSC
	#endif
#endif

#ifdef _MSC_VER
	#include <windows.h>
	#if !defined(MVSC)
		#define MVSC
	#endif
#else
	#include <unistd.h>
#endif

#if defined(__linux__) || defined(__GNUC__)
	#if !defined(GCCLINUX)
		#define GCCLINUX
	#endif
#endif

#if defined(MINGW)
	#undef GCCLINUX
#endif

#if defined(GCCLINUX) || defined(MINGW)

	#include <stdint.h>

#elif defined(MVSC)

	typedef unsigned char			uint8_t;
	typedef unsigned short int		uint16_t;
	typedef unsigned int			uint32_t;
	typedef unsigned __int64		uint64_t;

	typedef signed char				int8_t;
	typedef short int				int16_t;
	typedef int						int32_t;
	typedef __int64					int64_t;

#else
	#error OS not defined properly for 64 bit integers
#endif


//===============================================

struct ENC {
	double 	wscore;
	int		played;
	int 	wh;
	int 	bl;
};

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
