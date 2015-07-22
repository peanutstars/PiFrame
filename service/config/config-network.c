
#include <string.h>
#include <stdlib.h>

#include "pfdefine.h"
#include "pfconfig.h"
#include "pfunet.h"

#include "config-common.h"
#include "config-util.h"
#include "pfdebug.h"

/*****************************************************************************/

#define NET_DEFAULT_IP				"192.168.100.74"
#define NET_DEFAULT_NETMASK			"255.255.255.0"
#define NET_DEFAULT_GATEWAY			"192.168.100.1"
#define NET_DEFAULT_DNS1			"168.126.63.1"
#define NET_DEFAULT_DNS2			"8.8.8.8"

#define NETWORK_BASIC				"basic"
#define NETWORK_BASIC_INTERFACE		"interface"
#define NETWORK_BASIC_CONNECTION	"connection"
#define NETWORK_BASIC_DEV_WIRED		"devWired"

#define NETWORK_STATICIP			"staticIp"
#define NETWORK_STATICIP_IPADDR		"ipaddr"
#define NETWORK_STATICIP_NETMASK	"netmask"
#define NETWORK_STATICIP_GATEWAY	"gateway"
#define NETWORK_STATICIP_DNS1		"dns1"
#define NETWORK_STATICIP_DNS2		"dns2"


/*****************************************************************************/

static const char *basic_interface_table[EPFC_NI_COUNT] = {
	"NONE",
	"WIRED",
	"WIRELESS"
};

static const char *basic_type_table[EPFC_NC_COUNT] = {
	"DHCP",
	"STATIC"
};

/*****************************************************************************/

static void rf_network_basic(cJSON *node, struct PFConfig *config)
{
	char *jvalue;
	int i;

	/* interface */
	jvalue = json_getValueSibling (node, NETWORK_BASIC_INTERFACE);
	if ( jvalue ) {
		for ( i=0; i<EPFC_NI_COUNT; i++)
		{
			if ( ! strcasecmp(jvalue, basic_interface_table[i]) ) {
				config->network.basic.netif= i;
				break;
			}
		}
		jsonFree (jvalue);
	}
	DBG("Interface = %s\n", basic_interface_table[config->network.basic.netif]);

	/* type */
	jvalue = json_getValueSibling (node, NETWORK_BASIC_CONNECTION);
	if ( jvalue ) {
		for (i=0; i<EPFC_NC_COUNT; i++)
		{
			if ( ! strcasecmp(jvalue, basic_type_table[i]) ) {
				config->network.basic.type = i;
				break;
			}
		}
		jsonFree (jvalue);
	}
	DBG("Type = %s\n", basic_type_table[config->network.basic.type]);

	/* devWired */
	jvalue = json_getValueSibling (node, NETWORK_BASIC_DEV_WIRED);
	if (jvalue) {
		strcpy(config->network.basic.devWired, jvalue);
		jsonFree (jvalue);
	} else {
		strcpy(config->network.basic.devWired, "eth0");
	}
}

static void staticIp_setConfig (cJSON *node, const char *key, char *value, const char *defvalue)
{
	char *jvalue;
	int fgApplyDefValue = 1;

	jvalue = json_getValueSibling (node, key);
	if ( jvalue )
	{
		if ( PFU_isValidStrIPv4(jvalue) ) {
			strncpy (value, jvalue, MAX_ADDR_LENGTH);
			value[strlen(jvalue)] = '\0';
			fgApplyDefValue = 0;
		}
		else {
			fgApplyDefValue = 1;
		}
		jsonFree (jvalue);
	}

	if ( fgApplyDefValue )
		strcpy (value, defvalue);
}

static void rf_network_staticIp (cJSON *node, struct PFConfig *config)
{
	/* ipaddr */
	staticIp_setConfig (node, NETWORK_STATICIP_IPADDR, config->network.staticIp.ipaddr, NET_DEFAULT_IP) ;
//	DBG("ipaddr = %s\n", config->network.staticIp.ipaddr);

	/* netmask */
	staticIp_setConfig (node, NETWORK_STATICIP_NETMASK, config->network.staticIp.netmask, NET_DEFAULT_NETMASK) ;
//	DBG("netmask = %s\n", config->network.staticIp.netmask);

	/* gateway */
	staticIp_setConfig (node, NETWORK_STATICIP_GATEWAY, config->network.staticIp.gateway, NET_DEFAULT_GATEWAY) ;
//	DBG("gateway = %s\n", config->network.staticIp.gateway);

	/* dns */
	staticIp_setConfig (node, NETWORK_STATICIP_DNS1, config->network.staticIp.dns1, NET_DEFAULT_DNS1) ;
	staticIp_setConfig (node, NETWORK_STATICIP_DNS2, config->network.staticIp.dns2, NET_DEFAULT_DNS2) ;
	DBG("dns = %s, %s\n", config->network.staticIp.dns1, config->network.staticIp.dns2);
}

/******************************************************************************/

static void wf_network_basic (cJSON *root, struct PFConfig *config)
{
	cJSON *item = cJSON_CreateObject();

	ASSERT(config->network.basic.netif< EPFC_NI_COUNT);
	cJSON_AddStringToObject (item, NETWORK_BASIC_INTERFACE, basic_interface_table[config->network.basic.netif]);

	ASSERT(config->network.basic.type < EPFC_NC_COUNT);
	cJSON_AddStringToObject (item, NETWORK_BASIC_CONNECTION, basic_type_table[config->network.basic.type]);

	cJSON_AddStringToObject (item, NETWORK_BASIC_DEV_WIRED, config->network.basic.devWired);

	cJSON_AddItemToObject (root, NETWORK_BASIC, item);
}

static void wf_network_staticIp (cJSON *root, struct PFConfig *config)
{
	cJSON *item = cJSON_CreateObject();

	ASSERT(PFU_isValidStrIPv4(config->network.staticIp.ipaddr));
	cJSON_AddStringToObject (item, NETWORK_STATICIP_IPADDR, config->network.staticIp.ipaddr);

	ASSERT(PFU_isValidStrIPv4(config->network.staticIp.netmask));
	cJSON_AddStringToObject (item, NETWORK_STATICIP_NETMASK, config->network.staticIp.netmask);

	ASSERT(PFU_isValidStrIPv4(config->network.staticIp.gateway));
	cJSON_AddStringToObject (item, NETWORK_STATICIP_GATEWAY, config->network.staticIp.gateway);

	ASSERT(PFU_isValidStrIPv4(config->network.staticIp.dns1));
	cJSON_AddStringToObject (item, NETWORK_STATICIP_DNS1, config->network.staticIp.dns1);
	
	ASSERT(PFU_isValidStrIPv4(config->network.staticIp.dns2));
	cJSON_AddStringToObject (item, NETWORK_STATICIP_DNS1, config->network.staticIp.dns2);

	cJSON_AddItemToObject (root, NETWORK_STATICIP, item);
}

/******************************************************************************/

/**
 * @brief 네트워크 설정 읽기 테이블(read_table_network)
 */
struct ConfigHandler rt_network[] = {
	{ "basic",		NULL, rf_network_basic,		},
	{ "staticIp",	NULL, rf_network_staticIp,	},
	{ NULL, NULL, NULL, },
};

/**
 * @brief 네트워크 설정 쓰기 테이블(write_table_network)
 */
struct ConfigHandler wt_network[] = {
	{ "basic",		NULL, wf_network_basic, },
	{ "staticIp",	NULL, wf_network_staticIp, },
	{ NULL, NULL, NULL, },
};
