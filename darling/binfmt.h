#ifndef _BINFMT_H
#define _BINFMT_H
#include <linux/binfmts.h>

void macho_binfmt_init(void);
void macho_binfmt_exit(void);

#endif

