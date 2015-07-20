
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

struct PFWatchdog;

extern int fgDebugMode;

void watchdogInit (void);
void watchdogExit (void);

void watchdogAddEvent (const struct PFWatchdog *event);
void watchdogDump (void);

#endif /* __WATCHDOG_H__ */
