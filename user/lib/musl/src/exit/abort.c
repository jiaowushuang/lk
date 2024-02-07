#include <stdlib.h>
#include "atomic.h"

#include <compat_syscalls.h>

_Noreturn void abort(void)
{
	__sys_exit_etc(127, 0);

	/* Beyond this point should be unreachable. */
	a_crash();
	_Exit(127);
}
