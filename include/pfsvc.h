
#ifndef	__PF_SVC_H__
#define	__PF_SVC_H__

#ifdef WIN32
#else
 #include <arpa/inet.h>
#endif  // WIN32

#define	HTONS						htons
#define	HTONL						htonl
#define NTOHS						ntohs
#define NTOHL						ntohl

#define	SVC_DEFAULT_TIMEOUT			(5000)

/*****************************************************************************/


/*****************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

/*****************************************************************************/

enum EPF_MODULE {
	EPF_MOD_TEST,
	EPF_MOD_INIT,
	EPF_MOD_LOG,
	EPF_MOD_EVENT,
	EPF_MOD_SERVICE,
	EPF_MOD_CONFIG,
	EPF_MOD_ACTION,
	EPF_MOD_COUNT
} ;


#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__PF_SVC_H__*/
