#ifdef	__GNUC__
#    define alloca(x) __builtin_alloca (x)
#else
#if defined(HAVE_NO_ALLOCA) || defined(HAVE_NO_ALLOCA_H)

#ifdef X3J11
typedef void	*pointer;		/* generic pointer type */
pointer alloca (unsigned);		/* returns pointer to storage */
#else
typedef char	*pointer;		/* generic pointer type */
pointer alloca ();			/* returns pointer to storage */
#endif /* X3J11 */

#else

#include <alloca.h>

#endif
#endif
