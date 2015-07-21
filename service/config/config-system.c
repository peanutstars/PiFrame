
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "vmlimit.h"
#include "vmconfig.h"
#include "vmdefine.h"
//#include "logger.h"

#include "config-common.h"
#include "config-util.h"
#include "debug.h"

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
#define SYSTEM_INFO_PATH_CONFIG_GUI		"pathConfigGui"
#define SYSTEM_INFO_PATH_CONFIG			"pathConfig"
#define SYSTEM_INFO_BASE_ODD			"baseODD"
#define SYSTEM_INFO_BASE_ATA			"baseATA"
#define SYSTEM_INFO_BASE_USB			"baseUSB"
#define SYSTEM_INFO_BASE_NET			"baseNET"
#define SYSTEM_INFO_PART_ATA			"partATA"
#define SYSTEM_INFO_PREFIX_USB			"prefixUSB"
#define SYSTEM_ETC						"etc"
#define SYSTEM_ETC_FRONT_KEY_LOCK		"frontKeyLock"
#define SYSETM_ETC_RESERVATION			"reservation"
#define SYSTEM_ETC_POWER_ON_TIME		"powerOnTime"
#define SYSTEM_ETC_POWER_OFF_TIME		"powerOffTime"

/*****************************************************************************/

static void apply_hostname(const char *name)
{
	char *p;

	p = getenv (VM_DEF_BOOT_INFO_MODEL);
	if ( ! strcmp(p, "x86") ) {
		DBG("Skip to set hostname. (hostname = %s).\n", name);
		return;
	}

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

static void rf_system_name(cJSON *node, struct VMConfig *config)
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

static void rf_system_subModel (cJSON *node, struct VMConfig *config)
{
	char *jvalue = json_getValue (node);
	if (jvalue) {
		strncpy (config->system.subModel, jvalue, sizeof(config->system.subModel)-1);
		jsonFree (jvalue);
	}
}

static void rf_system_timezone(cJSON *node, struct VMConfig *config)
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
	
	config_load_maintain_tz(&config->system.tz);
	if (config_set_timezone (config->system.tz.id) < 0) {
		strcpy (config->system.tz.gmtOff, "9:00");
		strcpy (config->system.tz.zone, "KST");
		strcpy (config->system.tz.id, "Asia/Seoul");
		strcpy (config->system.tz.name, "Seoul");
		config_set_timezone (config->system.tz.id);
	}
#endif
}

static void rf_system_ntp(cJSON *node, struct VMConfig *config)
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


static void rf_system_info (cJSON *node, struct VMConfig *config)
{
	cJSON *item;
	char *jvalue;
	int i;

	json_setConfigStringValue (node, SYSTEM_INFO_PATH_DEF_CONFIG, config->system.info.pathDefConfig, sizeof(config->system.info.pathDefConfig));
	json_setConfigStringValue (node, SYSTEM_INFO_PATH_CONFIG_GUI, config->system.info.pathConfigGui, sizeof(config->system.info.pathConfigGui));
	json_setConfigStringValue (node, SYSTEM_INFO_PATH_CONFIG, config->system.info.pathConfig, sizeof(config->system.info.pathConfig));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_ODD, config->system.info.baseODD, sizeof(config->system.info.baseODD));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_ATA, config->system.info.baseATA, sizeof(config->system.info.baseATA));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_USB, config->system.info.baseUSB, sizeof(config->system.info.baseUSB));
	json_setConfigStringValue (node, SYSTEM_INFO_BASE_NET, config->system.info.baseNET, sizeof(config->system.info.baseNET));

	item = cJSON_GetObjectItem (node, SYSTEM_INFO_PART_ATA);
	for (i=0; i<MAX_ATA_PART_COUNT; i++) {
		if (i < cJSON_GetArraySize(item)) {
			jvalue = json_getValue (cJSON_GetArrayItem(item, i));
			if (jvalue) {
				strncpy (config->system.info.partATA[i], jvalue, sizeof(config->system.info.partATA[0])-1);
				DBG("partATA[%d] = %s\n", i, jvalue);
				jsonFree (jvalue);
			}
		}
	}

	json_setConfigStringValue (node, SYSTEM_INFO_PREFIX_USB, config->system.info.prefixUSB, sizeof(config->system.info.prefixUSB));
}

static void setConfigOnOff (const char *strValue, uint32_t *value)
{
	if ( ! strcasecmp("on", strValue) ) {
		*value = VM_MEDIA_ON;
	} else {
		*value = VM_MEDIA_OFF;
	}
}

static void rf_system_etc (cJSON *node, struct VMConfig *config)
{
	char *jvalue;
	uint8_t hour;
	uint8_t min;

	jvalue = json_getValueSibling (node, SYSTEM_ETC_FRONT_KEY_LOCK);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->system.etc.frontKeyLock);
		jsonFree(jvalue);
	} else {
		config->system.etc.frontKeyLock = VM_MEDIA_OFF;
	}

	jvalue = json_getValueSibling (node, SYSETM_ETC_RESERVATION);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->system.etc.reservation);
		jsonFree(jvalue);
	} else {
		config->system.etc.reservation = VM_MEDIA_OFF;
	}

	jvalue = json_getValueSibling (node, SYSTEM_ETC_POWER_ON_TIME);
	if (jvalue) {
		if ( 2 == sscanf(jvalue, "%hhu:%hhu", &hour, &min)) {
			config->system.etc.powerOn.hour = hour;
			config->system.etc.powerOn.min = min;
		}
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, SYSTEM_ETC_POWER_OFF_TIME);
	if (jvalue) {
		if ( 2 == sscanf(jvalue, "%hhu:%hhu", &hour, &min)) {
			config->system.etc.powerOff.hour = hour;
			config->system.etc.powerOff.min = min;
		}
		jsonFree (jvalue);
	}
}

/******************************************************************************/

static void wf_system_name(cJSON *root, struct VMConfig *config)
{
	cJSON_AddStringToObject(root, SYSTEM_NAME, config->system.name);
}

static void wf_system_subModel (cJSON *root, struct VMConfig *config)
{
	cJSON_AddStringToObject(root, SYSTEM_SUBMODEL, config->system.subModel);
}

static void wf_system_timezone(cJSON *root, struct VMConfig *config)
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

static void wf_system_ntp(cJSON *root, struct VMConfig *config)
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

static void wf_system_info (cJSON *root, struct VMConfig *config)
{
	cJSON *item = cJSON_CreateObject();
	const char *array[MAX_ATA_PART_COUNT];
	int i;

	cJSON_AddStringToObject(item, SYSTEM_INFO_PATH_DEF_CONFIG, config->system.info.pathDefConfig);
	cJSON_AddStringToObject(item, SYSTEM_INFO_PATH_CONFIG_GUI, config->system.info.pathConfigGui);
	cJSON_AddStringToObject(item, SYSTEM_INFO_PATH_CONFIG, config->system.info.pathConfig);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_ODD, config->system.info.baseODD);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_ATA, config->system.info.baseATA);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_USB, config->system.info.baseUSB);
	cJSON_AddStringToObject(item, SYSTEM_INFO_BASE_NET, config->system.info.baseNET);
	for (i=0; i<MAX_ATA_PART_COUNT; i++) {
		array[i] = config->system.info.partATA[i];
	}
	cJSON_AddItemToObject (item, SYSTEM_INFO_PART_ATA, 
			cJSON_CreateStringArray ((const char **)array, MAX_ATA_PART_COUNT));
	cJSON_AddStringToObject(item, SYSTEM_INFO_PREFIX_USB, config->system.info.prefixUSB);

	cJSON_AddItemToObject (root, SYSTEM_INFO, item);
}

static const char *strOnOff[2] = { "off", "on" };

static void wf_system_etc (cJSON *root, struct VMConfig *config)
{
	struct VMConfigSystemEtc *sysEtc = &config->system.etc;
	cJSON *item = cJSON_CreateObject();
	char tBuf[16];

	cJSON_AddStringToObject (item, SYSTEM_ETC_FRONT_KEY_LOCK,	strOnOff[sysEtc->frontKeyLock & 1]);
	cJSON_AddStringToObject (item, SYSETM_ETC_RESERVATION,		strOnOff[sysEtc->reservation & 1]);
	snprintf (tBuf, sizeof(tBuf), "%hhu:%hhu", sysEtc->powerOn.hour, sysEtc->powerOn.min);
	cJSON_AddStringToObject (item, SYSTEM_ETC_POWER_ON_TIME,	tBuf);
	snprintf (tBuf, sizeof(tBuf), "%hhu:%hhu", sysEtc->powerOff.hour, sysEtc->powerOff.min);
	cJSON_AddStringToObject (item, SYSTEM_ETC_POWER_OFF_TIME,	tBuf);

	cJSON_AddItemToObject (root, SYSTEM_ETC, item);
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
	{ "etc",		NULL, rf_system_etc, },
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
	{ "etc",		NULL, wf_system_etc, },
	{ NULL, NULL, NULL, },
};
