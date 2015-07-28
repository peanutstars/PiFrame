
#ifndef	__METHOD_H__
#define	__METHOD_H__

enum PFMethodType
{
	PFMT_NONE	= 0,
	PFMT_REPLY,
	PFMT_END
};

struct PFMethod {
	const char *name;
	int			minArgc;
	const char *help;
	void (*stub)(int argc, char **argv);
	int type;
};

#endif	/*__METHOD_H__*/
