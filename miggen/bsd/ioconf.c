#include <dev/busvar.h>


extern int pty_init(int);
extern int ptmx_init(int);
extern int vndevice_init(int);
extern int mdevinit(int);
extern int bpf_init(int);
extern int fsevents_init(int);
extern int random_init(int);

struct pseudo_init pseudo_inits[] = {
	{8,	pty_init},
	{1,	ptmx_init},
	{2,	vndevice_init},
	{1,	mdevinit},
	{4,	bpf_init},
	{1,	fsevents_init},
	{1,	random_init},
	{0,	0},
};
