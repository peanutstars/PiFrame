
#ifndef	__CONFIG_H__
#define	__CONFIG_H__

void configInit(const char *shmname);
struct PFConfig *configGet(void);
void configPut(int dirtyKlassMask);
void configUpdate(int force);
void configDefault(int mask);
int configExport(int mask, const char *dirpath);
int configImport(int mask, const char *path);

#endif	/*__CONFIG_H__*/
