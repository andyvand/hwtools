#include <stdio.h>
#include "test.h"

void x86emu_dump_xregs(void);

int int15_handler(void);

int int15_handler(void)
{
	printf("\nint15 encountered.\n");
	//x86emu_dump_xregs();
	X86_EAX = 0;
	return 1;
}
