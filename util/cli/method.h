
#ifndef	__METHOD_H__
#define	__METHOD_H__

enum VMMethodType
{
	VMMT_NONE	= 0,
	VMMT_REPLY,
	VMMT_END
};

struct VMMethod {
	const char *name;
	int			minArgc;
	const char *help;
	void (*stub)(int argc, char **argv);
	int type;
};

#endif	/*__METHOD_H__*/
