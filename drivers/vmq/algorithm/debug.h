#ifndef	__DEBUG_H__
#define	__DEBUG_H__

#ifdef	DEBUG

#ifdef	__KERNEL__

static inline const char *basename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail+1 : path;
}

#define DBG(str, args...)						\
	do { 								\
		printk("[%s:%s +%d] " str,				\
			THIS_MODULE->name,				\
			basename(__FILE__), __LINE__, ##args); 	\
	} while(0)

#define	__CHECK_LINE__							\
	do { 								\
		printk("[%s:%s:%s +%d]\n", 				\
			THIS_MODULE->name, 				\
			basename(__FILE__), __FUNCTION__, __LINE__); 	\
	} while(0)
#else	/*__KERNEL__*/

#include <stdio.h>
#include <libgen.h>

#define DBG(str, args...)						\
	do { 								\
		fprintf(stderr, "[%s +%d] " str,			\
			basename(__FILE__), __LINE__, ##args); 		\
	} while(0)

#define	__CHECK_LINE__							\
	do { 								\
		fprintf(stderr, "[%s:%s +%d]\n", 			\
			basename(__FILE__), __FUNCTION__, __LINE__); 	\
	} while(0)
#endif	/*__KERNEL__*/

#else	/*DEBUG*/

#define	DBG(...)	do { } while(0)
#define	__CHECK_LINE__	do { } while(0)

#endif	/*DEBUG*/

#endif	/*__DEBUG_H__*/
