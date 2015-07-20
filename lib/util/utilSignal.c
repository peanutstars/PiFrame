#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "pflimit.h"
#include "pfdebug.h"

/*****************************************************************************/

void PFU_registerSignal (int signum, void (*sigfunc)(int))
{
	struct sigaction act;

	act.sa_handler = sigfunc;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	act.sa_restorer = NULL;
	sigaction(signum, &act, 0); 
}
