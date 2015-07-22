#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "cJSON.h"
#include "pfdefine.h"
#include "pfconfig.h"
#include "config-common.h"
#include "config-util.h"
#include "pfdebug.h"


/*****************************************************************************/

/*
 * XXX : json_getValue(), json_getValuesSibling() 호출시, 
 *       return 값을 반드시 free() 해야 한다. !!!
 */
char *json_getValue (cJSON *item)
{
	char *jvalue = cJSON_Print(item);
	if (jvalue && item->type == cJSON_String)
	{
		/* remove double quotation */
		char *nJvalue = strdup(jvalue+1);
		nJvalue[strlen(nJvalue)-1] = '\0';
		jsonFree(jvalue);
		return nJvalue;
	}
	return jvalue;
}

char *json_getValueSibling (cJSON *root, const char *name)
{
	cJSON *sibling = root->child;
	char *jvalue=NULL;

	while (sibling)
	{   
		if ( ! strcmp(name, sibling->string)) {   
			jvalue = json_getValue (sibling);
			break;
		}   
		sibling = sibling->next;
	}   

	return jvalue;
}

void json_setConfigStringValue (cJSON *node, const char *name, char *buf, int bsize)
{
	char *jvalue;
	jvalue = json_getValueSibling (node, name);
	if (jvalue) {
		strncpy (buf, jvalue, bsize-1);
		jsonFree (jvalue);
	}
}

cJSON * json_getObject (cJSON *root, const char *name)
{
	cJSON *obj = root->child;

	while (obj)
	{
		if ( ! strcmp(name, obj->string) ) {
			return obj;
		}
		obj = obj->next;
	}
	return (cJSON *)NULL;
}

int json_saveFile (const char *path, cJSON *root, int format, int saveCount)
{
	char *printJson = NULL;
	char tmpPath[MAX_PATH_LENGTH];
	FILE *fp;
	int wsz = 0;

	/* Change Ping/Pong Logic - 2014.12.17
	   Save config data to method with ping and pong, 
	   cause of crashing config data when saving data with unpluged power. */

	if (format)
		printJson = cJSON_Print (root);
	else
		printJson = cJSON_PrintUnformatted (root);

	snprintf (tmpPath, sizeof(tmpPath), "%s.%d", path, saveCount&1);
	fp = fopen (tmpPath, "w");
	if (fp)
	{
		char msg[64];
		struct timeval tv;
		gettimeofday(&tv, NULL);	/* There is no connection with changing system time */
		snprintf (msg, sizeof(msg), "\nLastTime:%lu", tv.tv_sec);
		wsz = fprintf (fp, "%s", printJson);
		wsz += fprintf (fp, "%s", msg);
		fclose (fp);
//		rename (tmpPath, path);
	}

	if (printJson) {
		jsonFree (printJson);
	}
	return wsz;
}

/*****************************************************************************/

void config_read_iterate(cJSON *root, struct ConfigHandler *tbl, struct PFConfig *config)
{
	int i;
	cJSON *curr;

	for(i = 0; tbl[i].name != NULL; i++)
	{
		curr = json_getObject (root, tbl[i].name);
		if( curr)
		{
			if(tbl[i].handler)
				tbl[i].handler(curr, config);
			else {
				/* XXX : Not Tested */
				ASSERT(0 && "Not Tested");
				config_read_iterate(curr, tbl[i].child, config);
			}
		}
	}
}

void config_write_iterate (cJSON *root, struct ConfigHandler *tbl, struct PFConfig *config)
{
	struct ConfigHandler *curr = tbl;

	while (curr->name)
	{
		if (curr->child) {
			/* XXX : Not Tested */
			ASSERT(0 && "Not Tested");
			config_write_iterate (root, curr->child, config);
		}
		else
			curr->handler(root, config);

		curr ++;
	}
}

/*****************************************************************************/

int config_set_timezone (const char *tzId)
{
	char *basedir;
	char tzPath[MAX_PATH_LENGTH];
	char symTzPath[MAX_PATH_LENGTH];
	const char *tzLocaltime = "/etc/localtime";
	int rv = -1;
	
	basedir = getenv("VM_SYSTEM_BASE");
	if ( ! basedir) {
		basedir = "";
	}
	
	snprintf (tzPath, sizeof(tzPath), "%s/system/zoneinfo/%s", basedir, tzId);
	if ( ! access(tzPath, F_OK) ) {
		memset (symTzPath, 0, sizeof(symTzPath));
		readlink (tzLocaltime, symTzPath, sizeof(symTzPath));
		if (strcmp(tzPath, symTzPath)) {
			unlink (tzLocaltime);
			symlink (tzPath, tzLocaltime);
			sync();
			DBG("Change TZ from %s to %s.\n", symTzPath, tzPath);
		} else {
			DBG("Already applied TZ to %s.\n", tzPath);
		}
		rv = 0;
	} else {
		ERR2("Failed to set timezone, %s.\n", tzPath);
	}

	return rv;
}

