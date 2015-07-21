#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "cJSON.h"
#include "vmshm.h"
#include "pfdefine.h"
#include "pfconfig.h"
#include "pfufile.h"
//#include "vmutilSystem.h"
#include "pferror.h"

#include "config.h"
#include "config-common.h"
#include "config-util.h"
#include "notify.h"

#include "pfdebug.h"

/*****************************************************************************/

#ifndef PF_DEF_USER_CONFIG_DIR
#define	USER_CONFIG_DIR					"/data/config"
#else
#define	USER_CONFIG_DIR					PF_DEF_USER_CONFIG_DIR
#endif
#ifndef PF_DEF_DEFAULT_CONFIG_DIR
#define	DEFAULT_CONFIG_DIR				"/system/default"
#else
#define	DEFAULT_CONFIG_DIR				PF_DEF_DEFAULT_CONFIG_DIR
#endif
#ifndef PF_DEF_CONFIG_TMP_DIR
#define	CONFIG_TMP_DIR					"/tmp/config"
#else
#define	CONFIG_TMP_DIR					PF_DEF_CONFIG_TMP_DIR
#endif

#define	VMSHM_DEVICE					"/dev/vmshm"
#define	UPDATE_INTERVAL					(10)
#define	SEM_MAX_COUNT					(64)
#define	EXPORT_PREFIX					"Config"

/*****************************************************************************/

static int ConfigSemID;
static struct PFConfig *Config;


struct Database {
	const char *name;
	char path[MAX_PATH_LENGTH];
	int version;
	int revision;
	int dirty;
	int saveCount;
	struct ConfigHandler *readHandler;
	struct ConfigHandler *writeHandler;
};


/*****************************************************************************/

static struct Database databases[EPF_CONFIG_COUNT] = {
	{ 
		.name = "system", 
		.readHandler	= rt_system,
		.writeHandler	= wt_system,
		.saveCount		= 0,
	},
	{
		.name = "network",
		.readHandler	= rt_network,
		.writeHandler	= wt_network,
		.saveCount		= 0,
	},
	{
		.name = "runtime",
		.readHandler	= NULL,
		.writeHandler	= NULL,
		.saveCount		= 0,
	},
};


static struct timeval nextUpdate;

/*****************************************************************************/

static int timeDiff(const struct timeval *t1, const struct timeval *t2)
{
	int sec;
	int usec;

	sec = t1->tv_sec - t2->tv_sec;
	if(sec != 0)
		return abs(sec);

	usec = t1->tv_usec - t2->tv_usec;
	if(usec != 0)
		return abs(usec);

	return 0;
}

/*****************************************************************************/

static void semaInit(void)
{
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short int *array;
	} arg;

	DBG("==========================\n");
	DBG("Config Size = %zd\n", sizeof(*Config));
	DBG("==========================\n");

	ConfigSemID = semget((key_t)PF_CONFIG_KEY, 1, IPC_CREAT|IPC_EXCL|0660);
	if(ConfigSemID < 0) {
		if(errno != EEXIST) {
			ASSERT(! "semget must succeed, buf failed.");
		}
		ConfigSemID = semget((key_t)PF_CONFIG_KEY, 0, 0);
		ASSERT(ConfigSemID >= 0);
		/* 이미 존재 한다면 이전에 생성해 두었기 때문이다. 
		 * 초기화를 하지 않는다. */
		return;
	}

	/* 세마포어 초기화 */
	arg.val = SEM_MAX_COUNT;
	if(semctl(ConfigSemID, 0, SETVAL, arg) < 0) {
		ASSERT(! "semactl must succeed, buf failed.");
	}
}

static void semaLock(void)
{
	struct sembuf sb;

	/*
	 * NOTE
	 * SEM_MAX_COUNT로 하는 이유는 rwlock을 구현하기 위해서이다.
	 * binary sema(SEM_MAX_COUNT==1)일 경우 mutex로 구현되어 효율이 떨어진다.
	 * rwlock으로 구현시 효율이 상승한다.
	 *
	 * SEM_MAX_COUNT 변경시 ipcrm으로 sema를 제거해야 
	 * 다음 수행시 정상적으로 설정된다.
	 */
	sb.sem_num = 0;
	sb.sem_op = -SEM_MAX_COUNT;
	sb.sem_flg = SEM_UNDO;

	while(semop(ConfigSemID, &sb, 1) < 0) {
		if(errno != EINTR)
			goto err;
	}

	return;

err:
	ASSERT(! "semaLock failed.");
}

static void semaUnlock(void)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = SEM_MAX_COUNT;
	sb.sem_flg = SEM_UNDO;

	while(semop(ConfigSemID, &sb, 1) < 0) {
		if(errno != EINTR)
			goto err;
	}

	return;

err:
	ASSERT(! "semaUnock failed.");
}

/*****************************************************************************/


/**
 * @brief 각 클래스별로 읽은 doc을 이용하여 PFConfig를 공유 메모리에 구축한다.
 */
static void JsonToMem(struct Database *db, cJSON *root)
{
	ASSERT(root);
	config_read_iterate (root, db->readHandler, Config);
}

/**
 * @brief 공유 메모리에 있는 PFConfig를 JSON로 전환하여 돌려준다.
 */
static cJSON * MemToJson(struct Database *db)
{
	cJSON *root;

	root=cJSON_CreateObject();
	ASSERT(root);
	config_write_iterate(root, db->writeHandler, Config);

	return root;
}


static void rawCopyFile(int dst, int src)
{
	int rdcnt, wrcnt;
	char buffer[4096];

	for( ; ; )
	{
		rdcnt = PFU_safeRead(src, buffer, sizeof(buffer));
		if(rdcnt < 0)
			DIE("rawCopyFile failed\n");
		else if(rdcnt == 0)
			break;

		wrcnt = PFU_safeWrite(dst, buffer, rdcnt);

		if(wrcnt != rdcnt) {
			DIE("rawCopyFile failed\n");
		}
	}
}

#if	0
static void copyXML(const char *basedir, 
		const char *dbname, const char *dirpath)
{
	int fds, fdd;
	char src[256];
	char dst[256];

	sprintf(src, "%s/%s/%s.xml", basedir, USER_CONFIG_DIR, dbname);
	sprintf(dst, "%s/%s.xml", dirpath, dbname);

	if(access(src, F_OK) != 0) {
		LOG_ERR("%s: Default config not exist.", src);
		abort();
	}

	fds = open(src, O_RDONLY);
	ASSERT(fds > 0);

	fdd = open(dst, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	ASSERT(fdd > 0);

	rawCopyFile(fdd, fds);

	close(fds);
	close(fdd);
}
#endif	/*0*/

static void copyDefaultJson (const char *basedir, const char *dbname)
{

	int fds, fdd;
	char src[256];
	char dst[256];
	struct PFBaseInfo *base = &Config->base;

	sprintf(src, "%s/%s/%s/%s.json", basedir, DEFAULT_CONFIG_DIR, base->model, dbname);

	if( access(src, F_OK) != 0 ) {
//		LOG_ERR("%s: Default config not exist.", src);
		DIE("%s: Default config not exist.", src);
	}

	fds = open(src, O_RDONLY);
	ASSERT(fds > 0);

	sprintf(dst, "/bin/mkdir -p %s/%s", basedir, USER_CONFIG_DIR);
	system(dst);

	sprintf(dst, "%s/%s/%s.json", basedir, USER_CONFIG_DIR, dbname);
	fdd = open(dst, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	ASSERT(fdd > 0);

	rawCopyFile(fdd, fds);

	close(fds);
	close(fdd);

}

/*****************************************************************************/

static void baseInit (void)
{
	char *env;
	char *envRescue;
//	int ret;
	struct PFBaseInfo *base = &Config->base;

	/* INFO_RELEASE */
	env = getenv (PF_DEF_BOOT_INFO_RELEASE);
	ASSERT(env);
	base->release = atoi (env);

	/* INFO_VERSION */
	env = getenv (PF_DEF_BOOT_INFO_VERSION);
	ASSERT(env);
	envRescue = getenv (PF_DEF_BOOT_INFO_RESCUE);
	if (envRescue) {
		base->rescue = 1;
	}
	//strncpy (base->version, env, sizeof(base->version)-1);
	snprintf (base->version, sizeof(base->version), "%s%s", env, envRescue?".rescue":"");

	/* INFO_MODEL */
	env = getenv (PF_DEF_BOOT_INFO_MODEL);
	ASSERT(env);
	strncpy (base->model, env, sizeof(base->model)-1);

	DBG("BASE Information.\n");
	DBG("Release : %d\n", base->release);
	DBG("Version : %s\n", base->version);
	DBG("Rescue  : %d\n", base->rescue);
	DBG("Model   : %s\n", base->model);
}

static int getValidDBFile (struct Database *db, char *dbPath, int pathSize)
{
	char *p;
	FILE *fp;
	char cmd[256];
	snprintf (db->path, sizeof(db->path), USER_CONFIG_DIR "/%s.json", db->name);
	snprintf (cmd, sizeof(cmd), "/system/script/get_config %s", db->path);
	
	fp = popen (cmd, "r");
	if (fp) {
		fgets(dbPath, pathSize, fp);
		p = strrchr (dbPath, '\n');
		if (p) {
			*p = '\0';
		}
		pclose (fp);
	}

	db->saveCount = 0;
	p = strrchr(dbPath, '.');
	if (p && strcmp(p, ".json")) {
		sscanf(p, ".%d", &db->saveCount);
	}

	DBG("Get DB File : %s (%d)\n", dbPath, db->saveCount);
	return 0;
}

static void *getVaildDBContent (const char *basedir, struct Database *db, char *dbPath, int pathSize)
{
	char *jDoc = NULL;

	getValidDBFile (db, dbPath, pathSize);

	if ( access(dbPath, F_OK) != 0 ) {
		copyDefaultJson (basedir, db->name);
		getValidDBFile (db, dbPath, pathSize);
	}

	jDoc = PFU_readWholeFile (dbPath);

	return jDoc;
}

static void dbSync(int forceMask);
static int dbInit(const char *basedir)
{
	int i;
	int ret = -1;
	int retry;
	void *doc;
	cJSON *root;
	struct Database *db;
	int fgRetry = 0;
	char dbPath[MAX_BASE_PATH_LENGTH];

	for (i=0; i < EPF_CONFIG_COUNT; i++)
	{
#if 0
		if (i >= EPF_CT_RUNTIME_NOTUSED) {
			/* TODO - MUST be removed */
			continue;
		}
#endif

retry_file_error:
		db = &databases[i];

#if 0
		/* XXX : Save config data to method with ping and pong, 
		   cause of crashing config data when saving data with unpluged power. */
		snprintf (db->path, sizeof(db->path), "%s/%s/%s.json", basedir, USER_CONFIG_DIR, db->name);
		DBG("load %s\n", db->path);

		/* xxxx.json 파일이 없으면 default를 읽어 온다. */
		if ( access(db->path, F_OK) != 0 ) {
			copyDefaultJson(basedir, db->name);
		}

		retry = 0;
		doc = readWholeFile (db->path);
		while ( ! doc)
		{
//			LOG_ERR("%s read failed. Make default.\n", db->path);
			DBG("%s read failed, make default.\n", db->path);
			if (retry++ > 1)
				goto done;
			copyDefaultJson (basedir, db->name);
			doc = readWholeFile (db->path);
		}
#else
		retry = 0;
		do
		{
			dbPath[0]='\0';
			doc = getVaildDBContent (basedir, db, dbPath, sizeof(dbPath));
			if ( retry++ > 1 ) {
				ERR("retry to fail reading config file : %s\n", dbPath);
				goto done;
			}

		} while ( ! doc );
#endif
		
		root = cJSON_Parse(doc);
		// ASSERT(root && "JSON Parse error");
		if (root) {
			JsonToMem (db, root);
			cJSON_Delete (root);
		} else {
			ERR("%s file is wrong !!\n", dbPath);
			if (fgRetry++ < 3) {
				unlink (dbPath);
				free (doc);
				goto retry_file_error;
			}
		}
		db->saveCount ++;

		fgRetry = 0;
		free (doc);
	} /* for */
	ret = 0;

#if 0 /* for testing */
	dbSync(3);
	exit(EXIT_SUCCESS);
#endif
done:
	return ret;
}

/*
 * 공유 메모리에 있는 설정을 파일로 싱크한다.
 */
static void dbSync(int forceMask)
{
	int i;
	int bytes;
	cJSON *json;
	struct Database *db;

	for(i = 0; i < EPF_CONFIG_COUNT; i++)
	{
#if 0
		if (i >= EPF_CT_RUNTIME_NOTUSED) {
			/* TODO - MUST be removed */
			continue;
		}
#endif

		db = &databases[i];
		if(db->dirty > 0 || (forceMask & (1 << i)))
		{
			semaLock();
			json = MemToJson(db);
			semaUnlock();
			ASSERT(json);
			bytes = json_saveFile (db->path, json, 1, db->saveCount++);
			DBG("%s saved %d bytes.\n", db->path, bytes);
			db->dirty = 0;
			cJSON_Delete (json);
		}
	}
}

/*****************************************************************************/

/**
 * @brief struct PFConfig를 담을 공유 메모리를 설정한다.
 * @param name 부트업시 미리 생성된 공유 메모리 이름
 */
void configInit(const char *shmname)
{
	int fd;
	int ret;
	int size;
	char *basedir;

	basedir = getenv("PF_DATA_BASE");
	if(!basedir) {
		basedir = getenv("PF_SYSTEM_BASE");
		if(!basedir)
			basedir = "";
	}

	fd = open(VMSHM_DEVICE, O_RDWR);
	ASSERT(fd > 0 && VMSHM_DEVICE);

	PFU_setFD (fd, FD_CLOEXEC);

	size = VMSharedMemorySelect(fd, shmname);
	DBG("Shared Memory Size = %d\n", size);
	DBG("PFConfig Size      = %d\n", (int)sizeof(struct PFConfig));
	ASSERT(size > 0 && "VMSharedMemorySelect failed");

	Config = (struct PFConfig *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	ASSERT(MAP_FAILED != Config);

	memset(Config, 0, size);

	semaInit();
	baseInit();
	ret = dbInit(basedir);
	ASSERT(ret == 0 && "dbInit failed");

	if ( Config->system.subModel[0] == '\0' ) {
		strcpy (Config->system.subModel, Config->base.model);
	}
	Config->initialized = 1;
}

struct PFConfig *configGet(void)
{
	semaLock();
	return Config;
}

void configPut(int dirtyKlassMask)
{
	int i;
	struct Database *db;

	for(i = 0; i < EPF_CONFIG_COUNT; i++) {
		if(dirtyKlassMask & (1 << i)) {
			DBG("Dirty[%d]\n", i);
			db = &databases[i];
			db->revision++;
			db->dirty++;
		}
	}

	semaUnlock();
}


/**
 * @brief 주기적인 처리를 한다.
 */
void configUpdate(int force)
{
	struct timeval now;

	gettimeofday(&now, NULL);	/* There is no connection with changing system time */
//	DBG("now %ld , %ld, %d\n", now.tv_sec, nextUpdate.tv_sec, timeDiff(&now, &nextUpdate));
	if(force || (timeDiff(&now, &nextUpdate) >= UPDATE_INTERVAL)) {
		dbSync(force ? -1 : 0);
		nextUpdate = now;
	}
}

void configDefault(int mask)
{
}

int configExport(int mask, const char *path)
{
	int i;
	int ret;
	int rmask;
	char *basedir;
	char cmd[512];
	char tmp[128];
	struct Database *db;
	const struct PFBaseInfo *base = &Config->base;
	const char *model = base->model;

	dbSync(0);

	basedir = getenv("PF_SYSTEM_BASE");
	if(!basedir)
		basedir = "";

	sprintf(cmd, "/bin/mkdir -p %s/%s/", CONFIG_TMP_DIR, model);
	ret = system(cmd);
	if(!WIFEXITED(ret) || (WEXITSTATUS(ret) != 0)) {
//		LOG_SYS("Config export failed. Memory not enough.\n");
		ret = -PF_ERR_NOMEM;
		goto done;
	}

	sprintf(cmd, "/bin/cp -af ");
	for(i = rmask = 0; i < EPF_CONFIG_COUNT; i++) {
#if 0
		if (i >= EPF_CT_RUNTIME_NOTUSED) {
			/* TODO - MUST be removed */
			continue;
		}
#endif
		if(mask & (1 << i)) {
			db = &databases[i];
			sprintf(tmp, "%s/%s/%s.json ", basedir, USER_CONFIG_DIR, db->name);
			strcat(cmd, tmp);
			rmask |= (1 << i);
		}
	}

	if(!rmask) {
//		LOG_ERR("Export config mask is not set.\n");
		ret = -PF_ERR_INVALID;
		goto done;
	}

	sprintf(tmp, "%s/%s/", CONFIG_TMP_DIR, model);
	strcat(cmd, tmp);

	DBG("CMD=%s\n", cmd);
	ret = system(cmd);
	if(!WIFEXITED(ret) || (WEXITSTATUS(ret) != 0)) {
//		LOG_SYS("Config export failed. Memory not enough.\n");
		ret = -PF_ERR_NOMEM;
		goto done;
	}

	sprintf(cmd, "/bin/tar -cf \"%s\" -C %s %s", path, CONFIG_TMP_DIR, model);
	DBG("CMD=%s\n", cmd);
	ret = system(cmd);
	if(WIFEXITED(ret) && (WEXITSTATUS(ret) == 0)) {
		ret = access(path, F_OK);
		if(ret != 0) {
			ret = -1;
		}
	}
	else {
		ret = -1;
	}

	if(ret == 0) {
		DBG("Config exported to %s\n", path);
//		LOG_SYS("Config exported to %s\n", path);
	}
	else {
		DBG("Config export failed.\n");
//		LOG_ERR("Config export failed.\n");
	}

done:
	system("/bin/rm -rf " CONFIG_TMP_DIR);
	return ret;
}

int configImport(int mask, const char *path)
{
	int i;
	int rmask = 0;
	int ret = -PF_ERR_INVALID;
	char cmd[512];
	char tmp[128];
	char *basedir;
	struct Database *db;
	const struct PFBaseInfo *base = &Config->base;
	const char *model = base->model;

	dbSync(0);

	DBG("Import mask = 0x%x, path = %s\n", mask, path);

	system("/bin/mkdir -p " CONFIG_TMP_DIR);

	if(access(path, R_OK) != 0) {
		DBG("Config import file access failed.\n");
//		LOG_ERR("Config import failed. File cannot access.\n");
		ret = -PF_ERR_INVALID;
		goto done;
	}

	sprintf(cmd, "/bin/tar -xf %s -C " CONFIG_TMP_DIR, path);
	DBG("cmd = %s\n", cmd);
	ret = system(cmd);
	if(!WIFEXITED(ret) || (WEXITSTATUS(ret) != 0)) {
//		LOG_SYS("Config import failed. Invalid file.\n");
		DBG("Config import failed. Invalid file.\n");
		ret = -PF_ERR_INVALID;
		goto done;
	}

	sprintf(tmp, "%s/%s", CONFIG_TMP_DIR, model);
	if(access(tmp, F_OK) != 0) {
//		LOG_ERR("Config import failed. Different model's configuration.\n");
		DBG("Config import failed. Different model's configuration.\n");
		ret = -PF_ERR_INVALID_MODEL;
		goto done;
	}

	sprintf(cmd, "/bin/cp -af ");
	for(i = 0; i < EPF_CONFIG_COUNT; i++) {
#if 0
		if (i >= EPF_CT_RUNTIME_NOTUSED) {
			/* TODO - MUST be removed */
			continue;
		}
#endif
		if(mask & (1 << i)) {
			db = &databases[i];
			sprintf(tmp, "%s/%s/%s.json", CONFIG_TMP_DIR, model, db->name);
			DBG("tmp = %s\n", tmp);
			if(access(tmp, R_OK) == 0) {
				strcat(cmd, tmp);
				strcat(cmd, " ");
				rmask |= (1 << i);
			}
			else {
				perror(tmp);
			}
		}
	}

	DBG("Import rmask = 0x%x\n", rmask);
	if(rmask) {
		basedir = getenv("PF_SYSTEM_BASE");
		if(basedir)
			strcat(cmd, basedir);
		strcat(cmd, USER_CONFIG_DIR);
		DBG("Import Command: %s\n", cmd);
		ret = system(cmd);
		if(!WIFEXITED(ret) || (WEXITSTATUS(ret) != 0)) {
			DBG("Config import failed.\n");
//			LOG_ERR("Config import failed.\n");
			ret = -1;
		}
		else {
			DBG("Config imported from %s(mask = 0x%x)\n", path, rmask);
//			LOG_SYS("Config imported from %s(mask = 0x%x)\n", path, rmask);
			/* TODO : 설정 재로드에 따른 동작 수행 작업
					  재부팅 등등... */
			ret = 0;
		}
	}
	else {
		DBG("Config imported failed.\n");
//		LOG_SYS("Config imported failed.\n");
		ret = -1;
	}

done:
	system("/bin/rm -rf " CONFIG_TMP_DIR);
	return ret;
}

