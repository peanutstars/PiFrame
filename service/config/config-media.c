
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "vmconfig.h"
#include "vmdefine.h"
#include "vmplayer.h"
#include "vmutil.h"
#include "vmutilPlayer.h"
//#include "logger.h"

#include "config-common.h"
#include "config-util.h"
#include "debug.h"

/*****************************************************************************/

#define MEDIA_AUDIO						"audio"
#define AUDIO_BYPASS					"bypass"
#define AUDIO_PREVIEW_SOUND				"previewSound"
#define AUDIO_VOLUME					"volume"
#define AUDIO_MUTE						"mute"
#define MEDIA_VIDEO						"video"
#define VIDEO_PREVIEW_PLAY				"previewPlay"
#define VIDEO_3D_MODE					"_3DMode"
#define VIDEO_4K_FHD_MODE				"_4KFHDMode"
#define VIDEO_RESOLUTION				"resolution"
#define VIDEO_HDMI_EDID					"hdmiEdid"
#define VIDEO_HDMI_REFRESH				"hdmiRefresh"
#define VIDEO_HDMI_FAST_SWITCHING		"hdmiFastSwitching"
#define VIDEO_COLORSPACE				"colorspace"
#define VIDEO_SLIDE_TIME				"slideTime"
#define VIDEO_SLIDE_EFFECT				"slideEffect"
#define VIDEO_SEAMLESS					"seamless"
#define VIDEO_SYNC_PLAY_MODE			"syncPlayMode"
#define VIDEO_SYNC_PLAY					"syncPlay"
#define VIDEO_SYNC_PLAY_MASTER			"syncPlayMaster"
#define VIDEO_SYNC_CONNECT_UNIT_DEF		"syncConnectUnitDef"
#define VIDEO_CONTINUOUS_PLAY			"continuousPlay"
#define VIDEO_CROP_AT_CUSTOM_MODE		"cropAtCustomMode"
#define MEDIA_DISPLAY					"display"
#define DISPLAY_SHOW_RESOLUTION			"showResolution"
#define DISPLAY_OSD_MODE				"OSDMode"
#define DISPLAY_BRIGHTNESS				"brightness"
#define DISPLAY_CONTRAST				"contrast"
#define DISPLAY_SATURATION				"saturation"
#define MEDIA_SHARE						"share"
#define SHARE_SERVERIP					"serverIp"
#define SHARE_CONNECTION_TYPE			"connectionType"
#define SHARE_CONNECTION_PERIOD			"connectionPeriod"
#define MEDIA_ETC						"etc"
#define ETC_SPLITTER_EDID				"splitterEdid"
#define ETC_FUNCTION_LIMIT				"functionLimit"
#define ETC_FUNCTION_LIMIT_PASSWD		"functionLimitPasswd"
#define ETC_PLAYLIST_INEDX				"playlistIndex"
#define ETC_CONTENT_DEPLOY				"contentDeploy"
#define ETC_NULL_PACKET_LGE				"nullPacketLGE"

/*****************************************************************************/

static void setConfigOnOff (const char *strValue, uint32_t *value)
{
	if ( ! strcasecmp("on", strValue) ) {
		*value = VM_MEDIA_ON;
	} else {
		*value = VM_MEDIA_OFF;
	}
}

static void rf_media_audio (cJSON *node, struct VMConfig *config)
{
	char *jvalue;

	jvalue = json_getValueSibling (node, AUDIO_BYPASS);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.audio.bypass);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, AUDIO_PREVIEW_SOUND);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.audio.previewSound);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, AUDIO_VOLUME);
	if (jvalue) {
		config->media.audio.volume = strtoul(jvalue, NULL, 10);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, AUDIO_MUTE);
	if (jvalue) {
		config->media.audio.mute = VM_MEDIA_OFF;
		//setConfigOnOff (jvalue, &config->media.audio.mute);
		jsonFree (jvalue);
	}
}

static void rf_media_video (cJSON *node, struct VMConfig *config)
{
	char *jvalue;

	jvalue = json_getValueSibling (node, VIDEO_PREVIEW_PLAY);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.video.previewPlay);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_3D_MODE);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "none")) {
			config->media.video._3DMode = VM_MEDIA_3D_NONE;
		} else {
			config->media.video._3DMode = VM_MEDIA_3D_AUTO;
		}
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_4K_FHD_MODE);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "4k")) {
			config->media.video._4KFHDMode = VM_MEDIA_VIDEOMODE_4K;
		} else {
			config->media.video._4KFHDMode = VM_MEDIA_VIDEOMODE_FHD;
		}
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_RESOLUTION);
	if (jvalue) {
		config->media.video.resolution = VMUPlayerGetScreenFormat (jvalue);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_HDMI_EDID);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.video.hdmiEdid);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_HDMI_REFRESH);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "native")) {
			config->media.video.hdmiRefresh = VM_MEDIA_REFRESH_NATIVE;
		} else if ( ! strcasecmp(jvalue, "50&60")) {
			config->media.video.hdmiRefresh = VM_MEDIA_REFRESH_5060;
		} else if ( ! strcasecmp(jvalue, "60")) {
			config->media.video.hdmiRefresh = VM_MEDIA_REFRESH_60;
		} else if ( ! strcasecmp(jvalue, "50")) {
			config->media.video.hdmiRefresh = VM_MEDIA_REFRESH_50;
		} else {
			config->media.video.hdmiRefresh = VM_MEDIA_REFRESH_AUTO;
		}
		jsonFree (jvalue);
	}
	
	jvalue = json_getValueSibling (node, VIDEO_HDMI_FAST_SWITCHING);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.video.hdmiFastSwitching);
		jsonFree (jvalue);
	} else {
		config->media.video.hdmiFastSwitching = VM_MEDIA_OFF;
	}

	jvalue = json_getValueSibling (node, VIDEO_COLORSPACE);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "rgb 16~235")) {
			config->media.video.colorspace = VM_MEDIA_COLORSPACE_RGB_LIMITED;
		} else if ( ! strcasecmp(jvalue, "rgb 0~255")) {
			config->media.video.colorspace = VM_MEDIA_COLORSPACE_RGB_FULL;
		} else if ( ! strcasecmp(jvalue, "ycbcr")) {
			config->media.video.colorspace = VM_MEDIA_COLORSPACE_YCBCR;
		} else {
			config->media.video.colorspace = VM_MEDIA_COLORSPACE_AUTO;
		}
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_SLIDE_TIME);
	if (jvalue) {
		int slideTime = strtoul(jvalue, NULL, 10);
		if (slideTime < 10 || slideTime > 99) {
			slideTime = 10;
		}
		config->media.video.slideTime = slideTime;
		jsonFree (jvalue);
	} else {
		config->media.video.slideTime = 10;
	}

	jvalue = json_getValueSibling (node, VIDEO_SLIDE_EFFECT);
	if (jvalue) {
		/* TODO : Current Not Used */
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_SEAMLESS);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.video.seamless);
		jsonFree (jvalue);
	} else {
		config->media.video.seamless = VM_MEDIA_OFF;
	}

	config->media.video.syncPlayMode = VM_MEDIA_SYNC_PLAY_NONE;
	jvalue = json_getValueSibling (node, VIDEO_SYNC_PLAY_MODE);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "master")) {
			config->media.video.syncPlayMode = VM_MEDIA_SYNC_PLAY_MASTER;
		} else if ( ! strcasecmp(jvalue, "slave")) {
			config->media.video.syncPlayMode = VM_MEDIA_SYNC_PLAY_SLAVE;
		}
		jsonFree (jvalue);
	}

	/* for old data of config */
	jvalue = json_getValueSibling (node, VIDEO_SYNC_PLAY);
	if (jvalue) {
		uint32_t oSyncPlay;
		setConfigOnOff (jvalue, &oSyncPlay);
		jsonFree (jvalue);

		if (oSyncPlay == VM_MEDIA_ON) {
			jvalue = json_getValueSibling (node, VIDEO_SYNC_PLAY_MASTER);
			if (jvalue) {
				setConfigOnOff (jvalue, &oSyncPlay);
				jsonFree (jvalue);
				if (oSyncPlay == VM_MEDIA_ON) {
					config->media.video.syncPlayMode = VM_MEDIA_SYNC_PLAY_MASTER;
				} else {
					config->media.video.syncPlayMode = VM_MEDIA_SYNC_PLAY_SLAVE;
				}
			}
		}
	}

	jvalue = json_getValueSibling (node, VIDEO_SYNC_CONNECT_UNIT_DEF);
	if (jvalue) {
		uint32_t value = strtoul(jvalue, NULL, 10);
		config->media.video.syncConnectUnitDef = (value > 99) ? 99 : value;
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_CONTINUOUS_PLAY);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.video.continuousPlay);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, VIDEO_CROP_AT_CUSTOM_MODE);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.video.cropAtCustomMode);
		jsonFree (jvalue);
	} else {
		config->media.video.cropAtCustomMode = VM_MEDIA_OFF;
	}
}

static void rf_media_display (cJSON *node, struct VMConfig *config)
{
	char *jvalue;

	jvalue = json_getValueSibling (node, DISPLAY_SHOW_RESOLUTION);
	if (jvalue) {
		setConfigOnOff(jvalue, &config->media.display.showResolution);
		jsonFree(jvalue);
	}

	jvalue = json_getValueSibling (node, DISPLAY_OSD_MODE);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "show")) {
			config->media.display.OSDMode = VM_MEDIA_OSD_SHOW;
		} else if ( ! strcasecmp(jvalue, "special")) {
			config->media.display.OSDMode = VM_MEDIA_OSD_SPECIAL;
		} else {
			config->media.display.OSDMode = VM_MEDIA_OSD_ALL;
		}
		jsonFree(jvalue);
	}

	jvalue = json_getValueSibling (node, DISPLAY_BRIGHTNESS);
	if (jvalue) {
		config->media.display.brightness = strtoul (jvalue, NULL, 10);
		jsonFree(jvalue);
	}

	jvalue = json_getValueSibling (node, DISPLAY_CONTRAST);
	if (jvalue) {
		config->media.display.contrast= strtoul (jvalue, NULL, 10);
		jsonFree(jvalue);
	}

	jvalue = json_getValueSibling (node, DISPLAY_SATURATION);
	if (jvalue) {
		config->media.display.saturation = strtoul (jvalue, NULL, 10);
		jsonFree(jvalue);
	}
}

static void rf_media_share (cJSON *node, struct VMConfig *config)
{
	char *jvalue;

	jvalue = json_getValueSibling (node, SHARE_SERVERIP);
	if (jvalue) {
		if (VMUNetIsValidStrIPv4(jvalue)) {
			snprintf (config->media.share.serverIp, sizeof(config->media.share.serverIp), "%s", jvalue);
		} else {
			strcpy (config->media.share.serverIp, "192.168.0.1");
		}
		jsonFree(jvalue);
	} else {
		strcpy (config->media.share.serverIp, "192.168.0.1");
	}

	jvalue = json_getValueSibling (node, SHARE_CONNECTION_TYPE);
	if (jvalue) {
		if ( ! strcasecmp (jvalue, "samba") ) {
			config->media.share.connectionType = VM_MEDIA_SHARE_CONNECT_TYPE_SAMBA;
		} else if ( ! strcasecmp (jvalue, "network") ) {
			config->media.share.connectionType = VM_MEDIA_SHARE_CONNECT_TYPE_NETWORK;
		} else if ( ! strcasecmp (jvalue, "networkv2") ) {
			config->media.share.connectionType = VM_MEDIA_SHARE_CONNECT_TYPE_NETWORKV2;
		} else {
			config->media.share.connectionType = VM_MEDIA_SHARE_CONNECT_TYPE_NONE;
		}
		jsonFree(jvalue);
	}

	jvalue = json_getValueSibling (node, SHARE_CONNECTION_PERIOD);
	if (jvalue) {
		if ( ! strcasecmp (jvalue, "1min") ) {
			config->media.share.connectionPeriod = VM_MEDIA_SHARE_CONNECT_PERIOD_1MIN;
		} else if ( ! strcasecmp (jvalue, "10min") ) {
			config->media.share.connectionPeriod = VM_MEDIA_SHARE_CONNECT_PERIOD_10MIN;
		} else {
			config->media.share.connectionPeriod = VM_MEDIA_SHARE_CONNECT_PERIOD_1HOUR;
		}
		jsonFree(jvalue);
	}
}

static void rf_media_etc (cJSON *node, struct VMConfig *config)
{
	char *jvalue;

#if 1
	jvalue = json_getValueSibling (node, ETC_SPLITTER_EDID);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "copy")) {
			config->media.etc.splitterEdid = VM_MEDIA_SPLITTER_COPY;
		} else {
			config->media.etc.splitterEdid = VM_MEDIA_SPLITTER_COMMON;
		}
		jsonFree(jvalue);
	} else {
		config->media.etc.splitterEdid = VM_MEDIA_SPLITTER_COPY;
	}
#else
	config->media.etc.splitterEdid = VM_MEDIA_SPLITTER_COPY;
#endif

	jvalue = json_getValueSibling (node, ETC_FUNCTION_LIMIT);
	if (jvalue) {
		if ( ! strcasecmp(jvalue, "level 2")) {
			config->media.etc.functionLimit = VM_MEDIA_FUNCTION_LIMIT_L2;
		} else {
			config->media.etc.functionLimit = VM_MEDIA_FUNCTION_LIMIT_L1;
		}
	}

	jvalue = json_getValueSibling (node, ETC_FUNCTION_LIMIT_PASSWD);
	if (jvalue) {
		snprintf (config->media.etc.functionLimitPasswd, 
				sizeof(config->media.etc.functionLimitPasswd), "%s", jvalue);
		jsonFree(jvalue);
	}

	jvalue = json_getValueSibling (node, ETC_PLAYLIST_INEDX);
	if (jvalue) {
		config->media.etc.playlistIndex = strtoul (jvalue, NULL, 10);
		jsonFree (jvalue);
	}

	jvalue = json_getValueSibling (node, ETC_CONTENT_DEPLOY);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.etc.contentDeploy);
		jsonFree (jvalue);
	}

	/* add 20150129
	   LGE TV's issue - It has error on 2160p30 and 1080p24 cause of null packet */
	jvalue = json_getValueSibling (node, ETC_NULL_PACKET_LGE);
	if (jvalue) {
		setConfigOnOff (jvalue, &config->media.etc.nullPacketLGE);
		jsonFree (jvalue);
	} else {
		config->media.etc.nullPacketLGE = VM_MEDIA_OFF;
	}
}

/******************************************************************************/

static const char *strOnOff[2] = { "off", "on" };

static void wf_media_audio (cJSON *root, struct VMConfig *config)
{
	struct VMConfigMedia *media = &config->media;
	cJSON *item = cJSON_CreateObject();

	cJSON_AddStringToObject (item, AUDIO_BYPASS,		strOnOff[media->audio.bypass&1]);
	cJSON_AddStringToObject (item, AUDIO_PREVIEW_SOUND, strOnOff[media->audio.previewSound&1]);
	cJSON_AddNumberToObject (item, AUDIO_VOLUME,		media->audio.volume%101);
	cJSON_AddStringToObject (item, AUDIO_MUTE,			strOnOff[media->audio.mute&1]);

	cJSON_AddItemToObject (root, MEDIA_AUDIO, item);
}

static void wf_media_video (cJSON *root, struct VMConfig *config)
{
	struct VMConfigMedia *media = &config->media;
	cJSON *item = cJSON_CreateObject();
	const char *str3DMode[2] = { "none", "auto" };
	const char *str4KFHDMode[2] = { "fhd", "4k" };
	const char *strRefresh[5] = { "native", "auto", "50", "60", "50&60" };
	const char *strColorSpace[4] = { "auto", "ycbcr", "rgb 16~235", "rgb 0~255" };
	const char *strSyncPlayMode[4] = { "none", "master", "slave", "none" };

	cJSON_AddStringToObject (item, VIDEO_PREVIEW_PLAY,	strOnOff[media->video.previewPlay&1]);
	cJSON_AddStringToObject (item, VIDEO_3D_MODE,		str3DMode[media->video._3DMode&1]);
	cJSON_AddStringToObject (item, VIDEO_4K_FHD_MODE,	str4KFHDMode[media->video._4KFHDMode&1]);
	cJSON_AddStringToObject (item, VIDEO_RESOLUTION,	VMUPlayerGetStrScreenFormat(media->video.resolution));
	cJSON_AddStringToObject (item, VIDEO_HDMI_EDID,		strOnOff[media->video.hdmiEdid&1]);
	if (media->video.hdmiRefresh > VM_MEDIA_REFRESH_5060) {
		media->video.hdmiRefresh = VM_MEDIA_REFRESH_NATIVE;
	}
	cJSON_AddStringToObject (item, VIDEO_HDMI_REFRESH,	strRefresh[media->video.hdmiRefresh]);
	if (media->video.colorspace > VM_MEDIA_COLORSPACE_RGB_FULL) {
		media->video.colorspace = VM_MEDIA_COLORSPACE_AUTO;
	}
	cJSON_AddStringToObject (item, VIDEO_HDMI_FAST_SWITCHING, strOnOff[media->video.hdmiFastSwitching & 1]);
	cJSON_AddStringToObject (item, VIDEO_COLORSPACE,	strColorSpace[media->video.colorspace]);

	cJSON_AddNumberToObject (item, VIDEO_SLIDE_TIME,	media->video.slideTime);
	/* TODO : cJSON_AddStringToObject (item, VIDEO_SLIDE_EFFECT, media->video.slideEffect ); */
	cJSON_AddStringToObject (item, VIDEO_SEAMLESS,		strOnOff[media->video.seamless & 1]);
#if 0
	cJSON_AddStringToObject (item, VIDEO_SYNC_PLAY,		strOnOff[media->video.syncPlay & 1]);
	cJSON_AddStringToObject (item, VIDEO_SYNC_PLAY_MASTER, strOnOff[media->video.syncPlayMaster & 1]);
#else
	cJSON_AddStringToObject (item, VIDEO_SYNC_PLAY_MODE, strSyncPlayMode[media->video.syncPlayMode & 3]);
#endif
	cJSON_AddNumberToObject (item, VIDEO_SYNC_CONNECT_UNIT_DEF, media->video.syncConnectUnitDef);
	cJSON_AddStringToObject (item, VIDEO_CONTINUOUS_PLAY, strOnOff[media->video.continuousPlay & 1]);
	cJSON_AddStringToObject (item, VIDEO_CROP_AT_CUSTOM_MODE, strOnOff[media->video.cropAtCustomMode & 1]);

	cJSON_AddItemToObject (root, MEDIA_VIDEO, item);
}

static void wf_media_display (cJSON *root, struct VMConfig *config)
{
	struct VMConfigMedia *media = &config->media;
	cJSON *item = cJSON_CreateObject();
	const char *strOSDMode[3] = { "all", "show", "special" };

	cJSON_AddStringToObject (item, DISPLAY_SHOW_RESOLUTION,	strOnOff[media->display.showResolution]);
	if (media->display.OSDMode > VM_MEDIA_OSD_SPECIAL) {
		media->display.OSDMode = VM_MEDIA_OSD_ALL;
	}
	cJSON_AddStringToObject (item, DISPLAY_OSD_MODE,		strOSDMode[media->display.OSDMode]);
	cJSON_AddNumberToObject (item, DISPLAY_BRIGHTNESS,		media->display.brightness & 0xff);
	cJSON_AddNumberToObject (item, DISPLAY_CONTRAST,		media->display.contrast & 0xff);
	cJSON_AddNumberToObject (item, DISPLAY_SATURATION,		media->display.saturation & 0xff);

	cJSON_AddItemToObject (root, MEDIA_DISPLAY, item);
}

static void wf_media_share (cJSON *root, struct VMConfig *config)
{
	struct VMConfigMedia *media = &config->media;
	const char *strConnectionType[4] = { "none", "samba", "network", "networkv2" };
	const char *strConnectionPeriod[3] = { "1hour", "10min", "1min" };
	cJSON *item = cJSON_CreateObject ();

	cJSON_AddStringToObject (item, SHARE_SERVERIP, 
			(VMUNetIsValidStrIPv4(media->share.serverIp)) ? media->share.serverIp : "192.168.0.1");
	cJSON_AddStringToObject (item, SHARE_CONNECTION_TYPE, 
			(media->share.connectionType <= VM_MEDIA_SHARE_CONNECT_TYPE_NETWORKV2) ?
			strConnectionType[media->share.connectionType] : "none");
	cJSON_AddStringToObject (item, SHARE_CONNECTION_PERIOD,
			(media->share.connectionPeriod <= VM_MEDIA_SHARE_CONNECT_PERIOD_1MIN) ?
			strConnectionPeriod[media->share.connectionPeriod] : "1hour");

	cJSON_AddItemToObject (root, MEDIA_SHARE, item);
}

static void wf_media_etc (cJSON *root, struct VMConfig *config)
{
	struct VMConfigMedia *media = &config->media;
	cJSON *item = cJSON_CreateObject();
	const char *strSplitterEdid[2] = { "copy", "common" };
	const char *strFunctionLimit[2] = { "level 1", "level 2" };
	char functionLimitPasswd[9];

	cJSON_AddStringToObject (item, ETC_SPLITTER_EDID,		strSplitterEdid[media->etc.splitterEdid & 1]);
	cJSON_AddStringToObject (item, ETC_FUNCTION_LIMIT,		strFunctionLimit[media->etc.functionLimit & 1]);
	snprintf (functionLimitPasswd, sizeof(functionLimitPasswd), "%s", media->etc.functionLimitPasswd);
	cJSON_AddStringToObject (item, ETC_FUNCTION_LIMIT_PASSWD, functionLimitPasswd);
	cJSON_AddNumberToObject (item, ETC_PLAYLIST_INEDX,		media->etc.playlistIndex > 9 ? 0 : media->etc.playlistIndex);
	cJSON_AddStringToObject (item, ETC_CONTENT_DEPLOY,		strOnOff[media->etc.contentDeploy & 1]);
	cJSON_AddStringToObject (item, ETC_NULL_PACKET_LGE,		strOnOff[media->etc.nullPacketLGE & 1]);

	cJSON_AddItemToObject (root, MEDIA_ETC, item);
}

/******************************************************************************/

/**
 * @brief 시스템 설정 읽기 테이블(read_table_system)
 */
struct ConfigHandler rt_media[] = {
	{ "audio",		NULL, rf_media_audio, },
	{ "video",		NULL, rf_media_video, },
	{ "display",	NULL, rf_media_display, },
	{ "share",		NULL, rf_media_share },
	{ "etc",		NULL, rf_media_etc, },
	{ NULL, NULL, NULL, },
};

/**
 * @brief 시스템 설정 쓰기 테이블(write_table_system)
 */
struct ConfigHandler wt_media[] = {
	{ "audio",		NULL, wf_media_audio, },
	{ "video",		NULL, wf_media_video, },
	{ "display",	NULL, wf_media_display, },
	{ "share",		NULL, wf_media_share },
	{ "etc",		NULL, wf_media_etc, },
	{ NULL, NULL, NULL, },
};
