#ifndef _BINFMT_H
#define _BINFMT_H
#include <linux/binfmts.h>

extern struct linux_binfmt macho_format;

void macho_binfmt_init(void);
void macho_binfmt_exit(void);

#endif

