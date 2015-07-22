
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "pflimit.h"
#include "pfdefine.h"
#include "pfconfig.h"

#include "config-common.h"
#include "config-util.h"
#include "pfdebug.h"

/*****************************************************************************/

#define	HOSTNAME_PATH					"/etc/hostname"
#define SYSTEM_NAME						"name"
#define SYSTEM_SUBMODEL					"subModel"
#define SYSTEM_TZ						"timezone"
#define SYSTEM_TZ_GMTOFF				"gmtOff"
#define SYSTEM_TZ_ZONE					"zone"
#define SYSTEM_TZ_ID					"id"
#define SYSTEM_TZ_NAME					"name"
#define SYSTEM_NTP						"ntp"
#define SYSTEM_NTP_ENABLE				"enable"
#define SYSTEM_NTP_SERVERLISTS			"serverList"
#define SYSTEM_INFO						"info"
#define SYSTEM_INFO_PATH_DEF_CONFIG		"pathDefConfig"
#define SYSTEM_INFO_PATH_CONFIG			"pathConfig"
#define SYSTEM_INFO_BASE_ODD			"baseODD"
#define SYSTEM_INFO_BASE_ATA			"baseATA"
#define SYSTEM_INFO_BASE_USB			"baseUSB"
#define SYSTEM_INFO_BASE_NET			"baseNET"
#define SYSTEM_INFO_PREFIX_USB			"prefixUSB"

/*****************************************************************************/

static void apply_hostname(const char *name)
{
#if 0
	FILE *fp;
	fp = fopen(HOSTNAME_PATH, "w");
	if(fp) {
		fprintf(fp, "%s", name);
		fclose(fp);
		system("/bin/hostname -F " HOSTNAME_PATH);
	}
#else
	char cmd[128];

	snprintf (cmd, sizeof(cmd), "/bin/hostname %s", name);
	system (cmd);
#endif
}

static void rf_system_name(cJSON *node, struct PFConfig *config)
{	
	char *jvalue = json_getValue(node);
	if (jvalue)
	{
		if (config->initialized) {
			DBG("Player name is changed from %s to %s\n", config->system.name, jvalue);
		}
		strncpy (config->system.name, jvalue, sizeof(config->system.name)-1);
		config->system.name[strlen(jvalue)] = '\0';
		apply_hostname (jvalue);
		jsonFree (jvalue);
	}
}

static void rf_system_subModel (cJSON *node, struct PFConfig *config)
{
	char *jvalue = json_getValue (node);
	if (jvalue) {
		strncpy (config->system.subModel, jvalue, sizeof(config->system.subModel)-1);
		jsonFree (jvalue);
	}
}

static void rf_system_timezone(cJSON *node, struct PFConfig *config)
{
#if 0
	char *jvalue = json_getValue(node);
	if (jvalue)
	{
		int update = 0;
		if (config->system.timezone[0]) {
			if ( strcmp(config->system.timezone, jvalue) ) {
				DBG("Timezone changed from '%s' to '%s'.\n", config->system.timezone, jvalue);
				strncpy(config->system.timezone, jvalue, sizeof(config->system.timezone));
				config->system.timezone[strlen(jvalue)] = '\0';
				update = 1;
			}
		}
		else {
			strncpy(config->system.timezone, jvalue, sizeof(config->system.timezone));
			config->system.timezone[strlen(jvalue)] = '\0';
			update = 1;
		}
		if (update) {
			apply_timezone (config->system.timezone);
		}
		jsonFree (jvalue);
	}
#else
	char *jvalue;

	jvalue = json_getValueSibling (node, SYSTEM_TZ_GMTOFF);
	if (jvalue) {
		snprintf (config->system.tz.gmtOff, sizeof(config->system.tz.gmtOff), "%s", jvalue);
		jsonFree (jvalue);
	} else {
		strcpy (config->system.tz.gmtOff, "9:00");
	}
	jvalue = json_getValueSibling (node, SYSTEM_TZ_ZONE);
	if (jvalue) {
		snprintf (config->system.tz.zone, sizeof(config->system.tz.zone), "%s", jvalue);
		jsonFree (jvalue);
	} else {
		strcpy (config->system.tz.zone, "KST");
	}

	jvalue = json_getValueSibling (node, SYSTEM_TZ_ID);
	if (jvalue) {
		snprintf (config->system.tz.id, sizeof(config->system.tz.id), "%s", jvalue);
		jsonFree (jvalue);
	} else {
		strcpy (config->system.tz.id, "Asia/Seoul");
	}

	jvalue = json_getValueSibling (node, SYSTEM_TZ_NAME);
	if (jvalue) {
		snprintf (config->system.tz.name, sizeof(config->system.tz.name), "%s", jvalue);
		jsonFree (jvalue);
	} else {
		strcpy (config->system.tz.name, "Seoul");
	}
	
	if (config_set_timezone (config->system.tz.id) < 0) {
		strcpy (config->system.tz.gmtOff, "9:00");
		strcpy (config->system.tz.zone, "KST");
		strcpy (config->system.tz.id, "Asia/Seoul");
		strcpy (config->system.tz.name, "Seoul");
		config_set_timezone (config->system.tz.id);
	}
#endif
}

static void rf_system_ntp(cJSON *node, struct PFConfig *config)
{
	cJSON *item;
	char *jvalue;
	int i;

	jvalue = json_getValueSibling (node, SYSTEM_NTP_ENABLE);
	if (jvalue) {
		if ( ! strcmp("true", jvalue) )
			config->system.ntp.enable = 1;
		else
			config->system.ntp.enable = 0;
		jsonFree (jvalue);
	}

	item = cJSON_GetObjectItem (node, SYSTEM_NTP_SERVERLISTS);
	for (i=0; i<MAX_NTP_SERVER_COUNT; i++) {
		if (i < cJSON_GetArraySize(item)) {
			jvalue = json_getValue (cJSON_GetArrayItem (item, i));
			if (jvalue)
			{
				strncpy (config->system.ntp.server[i], jvalue, sizeof(config->system.ntp.server[0])-1);
				config->system.ntp.serverCount = i + 1;
				DBG("%d - %s\n", config->system.ntp.serverCount, config->system.ntp.server[i]);
				jsonFree (jvalue);
			}
		}
	}
}


static void rf_system_info (cJSON *node, struct PFConfig *config)
{
	json_setConfigStringValue (node, SYSTEM_INFO_PATH_DEF_CONFIG, config->system.info.pathDefConfig, sizeof(config->system.info.pathDefConfig));
	json_setConfigStringValue (node, SYSTEM_INFO_PATH_CONFIG, config->system.info.pathConfig, sizeof(config->system.info.pathConfig));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_ODD, config->system.info.baseODD, sizeof(config->system.info.baseODD));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_ATA, config->system.info.baseATA, sizeof(config->system.info.baseATA));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_USB, config->system.info.baseUSB, sizeof(config->system.info.baseUSB));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_NET, config->system.info.baseNET, sizeof(config->system.info.baseNET));

	json_setConfigStringValue (node, SYSTEM_INFO_PREFIX_USB, config->system.info.prefixUSB, sizeof(config->system.info.prefixUSB));
}

/******************************************************************************/

static void wf_system_name(cJSON *root, struct PFConfig *config)
{
	cJSON_AddStringToObject(root, SYSTEM_NAME, config->system.name);
}

static void wf_system_subModel (cJSON *root, struct PFConfig *config)
{
	cJSON_AddStringToObject(root, SYSTEM_SUBMODEL, config->system.subModel);
}

static void wf_system_timezone(cJSON *root, struct PFConfig *config)
{
#if 0
	cJSON_AddStringToObject(root, SYSTEM_TIMEZONE, config->system.timezone);
#else
	cJSON *item = cJSON_CreateObject();

	cJSON_AddStringToObject (item, SYSTEM_TZ_GMTOFF,	config->system.tz.gmtOff);
	cJSON_AddStringToObject (item, SYSTEM_TZ_ZONE,		config->system.tz.zone);
	cJSON_AddStringToObject (item, SYSTEM_TZ_ID,		config->system.tz.id);
	cJSON_AddStringToObject (item, SYSTEM_TZ_NAME,		config->system.tz.name);

	cJSON_AddItemToObject (root, SYSTEM_TZ, item);
#endif
}

static void wf_system_ntp(cJSON *root, struct PFConfig *config)
{
	cJSON *item = cJSON_CreateObject();
	const char *array[MAX_NTP_SERVER_COUNT];
	int i;

	if (config->system.ntp.enable) {
		cJSON_AddTrueToObject (item, SYSTEM_NTP_ENABLE);
	} else {
		cJSON_AddFalseToObject (item, SYSTEM_NTP_ENABLE);
	}

	for (i=0; i<config->system.ntp.serverCount; i++) {
		array[i] = config->system.ntp.server[i];
	}

	cJSON_AddItemToObject (item, SYSTEM_NTP_SERVERLISTS, 
			cJSON_CreateStringArray ((const char **)array, config->system.ntp.serverCount));

	cJSON_AddItemToObject (root, SYSTEM_NTP, item);
}

static void wf_system_info (cJSON *root, struct PFConfig *config)
{
	cJSON *item = cJSON_CreateObject();

	cJSON_AddStringToObject(item, SYSTEM_INFO_PATH_DEF_CONFIG, config->system.info.pathDefConfig);
	cJSON_AddStringToObject(item, SYSTEM_INFO_PATH_CONFIG, config->system.info.pathConfig);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_ODD, config->system.info.baseODD);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_ATA, config->system.info.baseATA);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_USB, config->system.info.baseUSB);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_NET, config->system.info.baseNET);
	cJSON_AddStringToObject(item, SYSTEM_INFO_PREFIX_USB, config->system.info.prefixUSB);

	cJSON_AddItemToObject (root, SYSTEM_INFO, item);
}

/******************************************************************************/

/**
 * @brief 시스템 설정 읽기 테이블(read_table_system)
 */
struct ConfigHandler rt_system[] = {
	{ "name",		NULL, rf_system_name, },
	{ "subModel",	NULL, rf_system_subModel, },
	{ "timezone",	NULL, rf_system_timezone, },
	{ "ntp",		NULL, rf_system_ntp, },
	{ "info",		NULL, rf_system_info, },
	{ NULL, NULL, NULL, },
};

/**
 * @brief 시스템 설정 쓰기 테이블(write_table_system)
 */
struct ConfigHandler wt_system[] = {
	{ "name",		NULL, wf_system_name, },
	{ "subModel",	NULL, wf_system_subModel, },
	{ "timezone",	NULL, wf_system_timezone, },
	{ "ntp",		NULL, wf_system_ntp, },
	{ "info",		NULL, wf_system_info, },
	{ NULL, NULL, NULL, },
};
