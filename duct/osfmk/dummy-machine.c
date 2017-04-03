/* 
 * dummy-machine.c
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <i386/machine_routines.h>


// i386/machine_routines.h

boolean_t machine_timeout_suspended(void) 
{
        kprintf("not implemented: machine_timeout_suspended()\n");
        return 0;
}

// boolean_t ml_set_interrupts_enabled(boolean_t enable)
// {
//         kprintf("not implemented: ml_set_interrupts_enabled(%d)\n", enable);
//         return 0;
// }

// osfmk/i386/machine_routines.c

unsigned int LockTimeOut;






