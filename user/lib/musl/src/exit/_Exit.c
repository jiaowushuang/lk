#include <stdlib.h>
#include <compat_syscalls.h>

_Noreturn void _Exit(int ec)
{
	for (;;) __sys_exit_etc(ec, 0);
}
