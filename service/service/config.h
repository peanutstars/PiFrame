#ifndef __CONFIG_H__
#define __CONFIG_H__

extern struct PFConfigSystem			configSystem;

void configUpdate(int eConfigType);
void configInit(void);
void configExit(void);

#endif
