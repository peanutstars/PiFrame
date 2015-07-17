
#ifndef	__NOTIFY_H__
#define	__NOTIFY_H__

#ifdef  __cplusplus
extern "C" {
#endif	/*__cplusplus*/

void notifyInit(void);
void notifyExit(void);
void notifyNoData (int type);
void notifyChar (unsigned char data);
void notifyShort (unsigned short data);
void notifyInt (unsigned int data);
void notifyLong (unsigned long data);

#ifdef	__cplusplus
};
#endif	/*__cplusplus*/

#endif	/*__NOTIFY_H__*/
