.globl _vectors

_vectors:

	ldr pc, =_boot_smp             /*                   offset 0 */
	ldr pc, =undef_instruction_exception_handler /* undef instruction offset 4 */
	ldr pc, =FreeRTOS_SWI_Handler               /* svc               offset 8 */
	ldr pc, =prefetch_abort_exception_handler    /* prefetch abort offset  0xc */
	ldr pc, =data_abort_exception_handler        /* data abort     offset 0x10 */
	nop
	ldr pc, =FreeRTOS_IRQ_Handler		 /* IRQ            offset 0x18 */
	ldr pc, =fiq_handler               /* FIQ            offset 0x1c */


