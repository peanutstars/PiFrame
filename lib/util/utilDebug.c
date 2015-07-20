
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pflimit.h"
#include "pfdebug.h"

#include "pfevent.h"

struct ResultMessage
{
	EResult		rv;
	char *		msg;
};


static const volatile struct ResultMessage resultTables[ERV_RESULT_COUNT] = {
	{	ERV_OK,					"OK"				},
	{	ERV_FAIL_UNKNOWN,		"Fail(unknown)"		},
	{	ERV_FAIL_BUSY,			"Fail(busy)"		},
	{	ERV_FAIL_NODEV,			"Fail(no device)"	},
	{	ERV_FAIL_PARAM,			"Fail(param)"		},
	{	ERV_FAIL_TIMEOUT,		"Fail(timeout)"		}
};

static const char *strUnknown = "Fail(unknown)";

const char *PFU_getStringERusult (EResult rv)
{
	int i;

	for (i=0; resultTables[i].msg; i++) {
		if ( rv == resultTables[i].rv ) {
			return resultTables[i].msg;
		}
	}
	return strUnknown;
}

void PFU_hexDump (const char *desc, const void *addr, const int len)
{
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*)addr;

	// Output description if given.
	if (desc != NULL)
		printf ("%s:\n", desc);

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).
		int modulo16 = i % 16;

		if (modulo16 == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0)
				printf ("  %s\n", buff);

			// Output the offset.
			printf ("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf (" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e))
			buff[modulo16] = '.';
		else
			buff[modulo16] = pc[i];
		buff[modulo16 + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf ("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf ("  %s\n", buff);
}

