#include "vmdefine.h"
#include "vmconfig.h"
#include "vmevent.h"
#include "vmdebug.h"
#include "config.h"
#include "notify.h"
#include "debug.h"

#include "config-util.h"
#include "config-runtime.h"

void updateConfigRuntimePlayer (const struct VMConfigRuntimePlayer *player)
{
	struct VMConfig *config = configGet ();
	int diff = 0;
	
	if ( memcmp(&config->runtime.player, player, sizeof(*player)) ) {
		memcpy (&config->runtime.player, player, sizeof(*player));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_PLAYER));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_RUNTIME_PLAYER);
	}
}

void updateConfigRuntimeEdid (const struct VMConfigRuntimeEdid *edid)
{
	struct VMConfig *config = configGet ();

	if (edid->size == 0) {
		config->runtime.edid.size = 0;
	} else {
		ASSERT ( !(edid->size > MAX_EDID_LENGTH));
		config->runtime.edid.size = edid->size;
		memcpy (config->runtime.edid.data, edid->data, 
				(edid->size > MAX_EDID_LENGTH) ? MAX_EDID_LENGTH : edid->size);
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_EDID));
	notifyConfigUpdate (VM_CONFIG_RUNTIME_EDID);
}

void updateConfigRuntimeStorage (const struct VMConfigRuntimeStorage *storage)
{
	struct VMConfig *config = configGet ();
	
	memcpy (&config->runtime.storage, storage, sizeof(*storage));

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_STORAGE));
	notifyConfigUpdate (VM_CONFIG_RUNTIME_STORAGE);
}

void updateConfigRuntimeActionCopy (const struct VMConfigRuntimeActionCopy *copy)
{
	struct VMConfig *config = configGet();

	memcpy (&config->runtime.action.copy, copy, sizeof(*copy));

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_ACTION));
	notifyConfigUpdate (VM_CONFIG_RUNTIME_ACTION);
}

void updateConfigRuntimeNetwork (const struct VMConfigRuntimeNetwork *network)
{
	struct VMConfig *config = configGet();

	memcpy (&config->runtime.network, network, sizeof(*network));

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_NETWORK));
	notifyConfigUpdate (VM_CONFIG_RUNTIME_NETWORK);
}

void updateConfigMedia  (const struct VMConfigMedia *media)
{
	struct VMConfig *config = configGet ();
	int diff = 0;
	
	if ( memcmp(&config->media, media, sizeof(*media)) ) {
		memcpy (&config->media, media, sizeof(*media));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_MEDIA));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_MEDIA);
		/* XXX : Remove /data/master, /data/sync and /data/pause incase of setting syncPlay to off Remove /data */
		if ( config->media.video.syncPlayMode == VM_MEDIA_SYNC_PLAY_NONE ) {
			unlink ("/data/sync");
			unlink ("/data/master");
			unlink ("/data/pause");
		}
	}
}

void updateConfigNetwork (const struct VMConfigNetwork *network)
{
	struct VMConfig *config = configGet ();
	int diff = 0;
	
	if ( memcmp(&config->network, network, sizeof(*network)) ) {
		memcpy (&config->network, network, sizeof(*network));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_NETWORK));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_NETWORK);
	}
}

void updateConfigRuntimeGUI (const struct VMConfigRuntimeGUI *gui)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	DBG("GUI Runtime Event !!! %d %d\n", gui->state, config->runtime.gui.state);
	if ( memcmp(&config->runtime.gui, gui, sizeof(*gui)) ) {
		memcpy (&config->runtime.gui, gui, sizeof(*gui));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_GUI));
	if (diff) {
		DBG("GUI Runtime Diff !!!\n");
		notifyConfigUpdate (VM_CONFIG_RUNTIME_GUI);
	}
}

void updateConfigRuntimeSFT (const struct VMConfigRuntimeSFT *sft)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	if ( memcmp(&config->runtime.sft, sft, sizeof(*sft)) ) {
		memcpy (&config->runtime.sft, sft, sizeof(*sft));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_SFT));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_RUNTIME_SFT);
	}
}

void updateConfigSystemEtc (const struct VMConfigSystemEtc *sysEtc)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	if ( memcmp(&config->system.etc, sysEtc, sizeof(*sysEtc))) {
		memcpy (&config->system.etc, sysEtc, sizeof(*sysEtc));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_SYSTEM));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_SYSTEM);
	}
}

void updateConfigSystemTimezone (const struct VMConfigSystemTimezone *tz)
{
	struct VMConfig *config = configGet();
	int diff = 0;

	if ( memcmp(&config->system.tz, tz, sizeof(*tz)) ) {
		memcpy (&config->system.tz, tz, sizeof(*tz));
		diff = 1;
		config_save_maintain_tz (tz);
		config_set_timezone (config->system.tz.id);
	}
	configPut (VM_CONFIG_MASK(VM_CONFIG_SYSTEM));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_SYSTEM);
	}
}

void updateConfigRuntimeMicom (const struct VMConfigRuntimeMicom *micom)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	if ( memcmp(&config->runtime.micom, micom, sizeof(*micom)) ) {
		memcpy (&config->runtime.micom, micom, sizeof(*micom));
		diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_MICOM));
	if (diff) {
		notifyConfigUpdate (VM_CONFIG_RUNTIME_MICOM);
	}
}

void updateConfigRuntimeNDS (const struct VMConfigRuntimeNDS *nds)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	if( memcmp(&config->runtime.nds, nds, sizeof(*nds)) ) {
	      memcpy (&config->runtime.nds, nds, sizeof(*nds));
	      diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_NDS));
	if (diff) {
	      notifyConfigUpdate (VM_CONFIG_RUNTIME_NDS);
	}
}

void updateConfigRuntimeNDS_Matrix (const struct VMConfigRuntimeNDS_Matrix *matrix)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	if( memcmp(&config->runtime.matrix, matrix, sizeof(*matrix)) ) {
	      memcpy (&config->runtime.matrix, matrix, sizeof(*matrix));
	      diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_NDS_MAT));
	if (diff) {
	      notifyConfigUpdate (VM_CONFIG_RUNTIME_NDS_MAT);
	}
}

//void updateConfigRuntimeNDS_State (const struct VMEDzNDSConfigState *state)
void updateConfigRuntimeNDS_State (const struct VMConfigRuntimeNDS_State *state)
{
	struct VMConfig *config = configGet ();
	int diff = 0;

	if( memcmp(&config->runtime.nds_state, state, sizeof(*state)) ) {
	      memcpy (&config->runtime.nds_state, state, sizeof(*state));
	      diff = 1;
	}

	configPut (VM_CONFIG_MASK(VM_CONFIG_RUNTIME_NDS_STATE));
	if (diff) {
	      notifyConfigUpdate (VM_CONFIG_RUNTIME_NDS_STATE);
	}
}


