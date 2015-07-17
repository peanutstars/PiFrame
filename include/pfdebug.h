
#ifndef	__PFDEBUG_H__
#define	__PFDEBUG_H__

#ifdef	__KERNEL__

static inline const char *__basename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail+1 : path;
}

#ifdef	DEBUG

#define DBG(str, args...)						\
	do { 								\
		printk("[%s:%s +%d] " str,				\
			THIS_MODULE->name,				\
			__basename(__FILE__), __LINE__, ##args); 	\
	} while(0)

#define	__CHECK_LINE__							\
	do { 								\
		printk("[%s:%s:%s +%d]\n", 				\
			THIS_MODULE->name, 				\
			__basename(__FILE__), __FUNCTION__, __LINE__); 	\
	} while(0)

#define	ERR(str, args...)	\
	do { 								\
		printk(KERN_ERR "[%s:%s +%d] " str,			\
			THIS_MODULE->name,				\
			__basename(__FILE__), __LINE__, ##args); 	\
	} while(0)
#else	/*DEBUG*/

#define	DBG(...)	do { } while(0)
#define	__CHECK_LINE__	do { } while(0)
#define	ERR(str, args...)	\
	do { 								\
		printk(KERN_ERR "[%s:%s +%d] " str,			\
			THIS_MODULE->name,				\
			__basename(__FILE__), __LINE__, ##args); 	\
	} while(0)

#endif	/*DEBUG*/

#else	/*__KERNEL__*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#define HL_RED            "\x1b[00;031;031m"
#define HL_GREEN          "\x1b[00;032;032m"
#define HL_YELLOW         "\x1b[00;033;033m"
#define HL_BLUE           "\x1b[00;034;034m"
#define HL_PURPLE         "\x1b[00;035;035m"
#define HL_CYAN           "\x1b[00;036;036m"
#define HL_WHITE          "\x1b[00;037;037m"
#define HL_NONE	          "\x1b[00m"

static inline const char *__basename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail+1 : path;
}

#ifdef	DEBUG

#define DBG(str, args...)									\
	do { 													\
		fprintf(stderr, "[%ld:%s +%d] " str,				\
			time(NULL), __basename(__FILE__), __LINE__, ##args); 		\
		fflush(stderr);										\
	} while(0)

#define	ERR(str, args...)									\
	do { 													\
		fprintf(stderr, HL_RED "[%ld:%s +%d] " str HL_NONE,	\
			time(NULL), __basename(__FILE__), __LINE__, ##args); 		\
	} while(0)

#define	ERR2(str, args...)									\
	do { 													\
		fprintf(stderr, HL_RED "[%ld:%s +%d] E[%d:%s] " str HL_NONE,	\
			time(NULL), __basename(__FILE__), __LINE__, errno, strerror(errno), ##args); 		\
		fflush(stderr);										\
	} while(0)

#define	__CHECK_LINE__										\
	do { 													\
		fprintf(stderr, "[%ld:%s:%s +%d]\n", 					\
			time(NULL), __basename(__FILE__), __FUNCTION__, __LINE__); 	\
		fflush(stderr);										\
	} while(0)


#define	ASSERT(expr) \
	((expr) ? (int)sizeof(int) : __die( # expr, __FILE__, __LINE__))

#define ESTIMATE_START(title) \
	do { \
		unsigned int __diff_usec; \
		unsigned int __boundary = 0; \
		const char *__estimate_title = title; \
		struct timeval __s_tv, __e_tv; \
		gettimeofday(&__s_tv, (struct timezone *)0)

#define ESTIMATE_START_BOUND(title, b) \
	do { \
		unsigned int __diff_usec; \
		unsigned int __boundary = b; \
		const char *__estimate_title = title; \
		struct timeval __s_tv, __e_tv; \
		gettimeofday(&__s_tv, (struct timezone *)0)

#define ESTIMATE_STOP() \
		gettimeofday(&__e_tv, (struct timezone *)0); \
		__diff_usec = (__e_tv.tv_sec - __s_tv.tv_sec) * 1000000; \
		__diff_usec += (__e_tv.tv_usec - __s_tv.tv_usec); \
		if(__diff_usec >= __boundary) \
		DBG("ESTIMATED TIME[%s] : %d(usec)\n", \
				__estimate_title, __diff_usec); \
	} while(0)

#define DIE(str, args...)	\
	do { \
		int __a; \
		int *__abortPtr = (int *)0; \
		fprintf(stderr, HL_RED "[%ld:%s +%d] " str HL_NONE, \
			time(NULL), __basename(__FILE__), __LINE__, ##args); \
		fflush(stderr);										\
		fflush(stderr);										\
		__a = *__abortPtr; \
		fprintf(stderr,"__a=%X\n", __a); \
	} while(0)

static inline int __die(const char *expr, const char *file, int line)
{
	int __a;
	int *__abortPtr = (int *)0;
	fprintf(stderr, HL_RED "[%ld:%s +%d] ASSERTION FAILED: %s\n" HL_NONE,
			time(NULL), __basename(file), line, expr);
	fflush(stderr);
	fflush(stderr);
	__a = *__abortPtr;
	return __a;
}

#define	HI_CHECK_ERROR(expr)					\
	do {										\
		int __hi_ret;							\
		__hi_ret = expr;						\
		if(__hi_ret) {							\
			DIE("ERROR: 0x%x\n", __hi_ret);		\
		}										\
	} while(0)

#define	HI_CHECK_ERROR_DEBUG(expr)				\
	do {										\
		int __hi_ret;							\
		__hi_ret = expr;						\
		if(__hi_ret) {							\
			DBG("ERROR: 0x%x\n", __hi_ret);		\
		}										\
	} while(0)

#else	/*DEBUG*/

#define	DBG(...)	do { } while(0)
#define	__CHECK_LINE__	do { } while(0)
#define	ERR(str, args...)	\
	do { 								\
		fprintf(stderr, HL_RED "[%s +%d] " str HL_NONE,		\
			__basename(__FILE__), __LINE__, ##args); 		\
		fflush(stderr);										\
	} while(0)
#define	ERR2(str, args...)	\
	do { 								\
		fprintf(stderr, HL_RED "[%s +%d] E[%d:%s] " str HL_NONE,			\
			__basename(__FILE__), __LINE__, errno, strerror(errno), ##args); 		\
		fflush(stderr);										\
	} while(0)

#define ESTIMATE_START(...)				do { } while(0)
#define ESTIMATE_START_BOUND(...)		do { } while(0)
#define ESTIMATE_STOP(...)				do { } while(0)
#define DIE(str, args...)	\
	do { \
		int __a; \
		int *__abortPtr = (int *)0; \
		fprintf(stderr, HL_RED "[%ld:%s +%d] " str HL_NONE, \
			time(NULL), __basename(__FILE__), __LINE__, ##args); \
		fflush (stderr); \
		fflush (stderr); \
		__a = *__abortPtr; \
	} while(0)

#define	HI_CHECK_ERROR(expr)					\
	do {										\
		int __hi_ret;							\
		__hi_ret = expr;						\
		if(__hi_ret) {							\
			DIE("ERROR: 0x%x\n", __hi_ret);		\
		}										\
	} while(0)
#define	HI_CHECK_ERROR_DEBUG(expr)		(expr)
#define	ASSERT(...) 					do { } while(0)

#endif	/*DEBUG*/

#endif	/*__KERNEL__*/

#endif	/*__PFDEBUG_H__*/
