
#include <kern/asm.h>
#include <arch/asm_macros.h>
#include <arch/arch_macro.h>
#include <arch/cpu_ops.h>
#include <assym.h>

FUNCTION(init_cpu_ops) 
	mrs	x6, TPIDR_ELX
	ldr	x0, [x6, #PCPU_KDATA_OPS_OFFSET]
	cbnz	x0, 1f
	mov	x10, x30
	bl	get_cpu_ops_ptr
#if ENABLE_ASSERTIONS
	cmp	x0, #0
	ASM_ASSERT(ne)
#endif
	str	x0, [x6, #PCPU_KDATA_OPS_OFFSET]!
	mov x30, x10
1:
	ret

/* midr diff core types */
FUNCTION(get_cpu_ops_ptr) 
	/* Read the MIDR_EL1 */
	mrs	x2, MIDR_ELX
	mov_imm	x3, CPU_IMPL_PN_MASK

	/* Retain only the implementation and part number using mask */
	and	w2, w2, w3

	/* Get the cpu_ops end location */
	adr	x5, (__CPU_OPS_END__ + CPU_MIDR)

	/* Initialize the return parameter */
	mov	x0, #0
1:
	/* Get the cpu_ops start location */
	adr	x4, (__CPU_OPS_START__ + CPU_MIDR)

2:
	/* Check if we have reached end of list */
	cmp	x4, x5
	b.eq	search_def_ptr

	/* load the midr from the cpu_ops */
	ldr	x1, [x4], #CPU_OPS_SIZE
	and	w1, w1, w3

	/* Check if midr matches to midr of this core */
	cmp	w1, w2
	b.ne	2b

	/* Subtract the increment and offset to get the cpu-ops pointer */
	sub	x0, x4, #(CPU_OPS_SIZE + CPU_MIDR)
#if ENABLE_ASSERTIONS
	cmp	x0, #0
	ASM_ASSERT(ne)
#endif
search_def_ptr:
	ret

FUNCTION(do_cpu_reg_dump) 
	mov	x16, x30

	/* Get the matching cpu_ops pointer */
	bl	get_cpu_ops_ptr
	cbz	x0, 1f

	/* Get the cpu_ops cpu_reg_dump */
	ldr	x2, [x0, #CPU_REG_DUMP]
	cbz	x2, 1f
	blr	x2
1:
	mov	x30, x16
	ret
