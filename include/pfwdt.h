
#ifndef __PFWDT_H__
#define __PFWDT_H__

#ifdef	__cplusplus
extern "C" {
#endif	/*__cplusplus*/

struct PFWdtHandler;

int PFWatchdogRegister (int argc, char *argv[], emode_vmwd_t eMode, int timeout);
int PFWatchdogUnregister (void);
int PFWatchdogStart (void);
int PFWatchdogStop (void);
int PFWatchdogAlive (void);

#if 0
int PFWatchdog_subRegister (uint32_t *subId, int timeout);
int PFWatchdog_subUnregister (uint32_t subId);
int PFWatchdog_subAlive (uint32_t subId);
#endif

struct PFWdtHandler *PFWatchdogSubRegister (int timeout);
int PFWatchdogSubUnregister (struct PFWdtHandler *handle);
int PFWatchdogSubAlive (struct PFWdtHandler *handle, int period);

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif /* __PFWDT_H__ */
