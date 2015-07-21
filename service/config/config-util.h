
#ifndef	__CONFIG_UTIL_H__
#define	__CONFIG_UTIL_H__

#include <cJSON.h>

#include "pfconfig.h"

/*****************************************************************************/

#define	_X								
#define	ATOI(expr)						atoi((const char *)(expr))

#define	TABLE_COUNT(tbl)				(sizeof(tbl) / sizeof(tbl[0]))

/*****************************************************************************/

struct ConfigHandler;

/*****************************************************************************/

#define jsonFree		free

/*****************************************************************************
 * XXX : Must be called free() 
 */
char *json_getValue (cJSON *obj);
char *json_getValueSibling (cJSON *root, const char *name);

/*****************************************************************************/

void json_setConfigStringValue (cJSON *node, const char *name, char *buf, int bsize);
cJSON * json_getObject (cJSON *root, const char *name);
int json_saveFile (const char *path, cJSON *root, int format, int saveCount);

/*****************************************************************************/

void config_read_iterate  (cJSON *, struct ConfigHandler *, struct PFConfig *);
void config_write_iterate (cJSON *, struct ConfigHandler *, struct PFConfig *);

/*****************************************************************************/

struct VMConfigSystemTimezone;

int config_set_timezone (const char *tzId);
void config_save_maintain_tz (const struct VMConfigSystemTimezone *tz);
int config_load_maintain_tz (struct VMConfigSystemTimezone *tz);

#endif	/*__SETUP_UTIL_H__*/
