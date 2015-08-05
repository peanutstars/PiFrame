#ifndef ___SERVICE_H___
#define ___SERVICE_H___

struct PFEvent ;

void serviceInit(void) ;
void serviceExit(void) ;
void serviceAddQueue (struct PFEvent *event) ;

#endif /* ___SERVICE_H___ */
