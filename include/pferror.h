
#ifndef	__PF_ERROR_H__
#define	__PF_ERROR_H__

#ifdef  __cplusplus
extern "C" {
#endif  /*__cplusplus*/

enum {
	PF_OK,
	PF_ERR_INVALID = 256,
	PF_ERR_NOMEM,
	PF_ERR_PERM,
	PF_ERR_NOENT,
	PF_ERR_EXEC,
	PF_ERR_BUSY,
	PF_ERR_CONFIG_REVISION_MISMATCH,
	PF_ERR_AUTH,
	PF_ERR_LIMIT,
	PF_ERR_INVALID_MODEL,
};

#ifdef  __cplusplus
};
#endif  /*__cplusplus*/

#endif	/*__PF_ERROR_H__*/
