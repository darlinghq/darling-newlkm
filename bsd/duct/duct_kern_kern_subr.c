#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/bsd/kern/kern_subr.c">

/*
 * General routine to allocate a hash table.
 */
void *
hashinit(int elements, int type, u_long *hashmask)
{
	long hashsize;
	LIST_HEAD(generic, generic) * hashtbl;
	int i;

	if (elements <= 0) {
		panic("hashinit: bad cnt");
	}
	for (hashsize = 1; hashsize <= elements; hashsize <<= 1) {
		continue;
	}
	hashsize >>= 1;
	MALLOC(hashtbl, struct generic *,
	    hashsize * sizeof(*hashtbl), type, M_WAITOK | M_ZERO);
	if (hashtbl != NULL) {
		for (i = 0; i < hashsize; i++) {
			LIST_INIT(&hashtbl[i]);
		}
		*hashmask = hashsize - 1;
	}
	return hashtbl;
}

// </copied>
