/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/* *INDENT-OFF* */
.macro push ra, rb
stp \ra, \rb, [sp,#-16]!
.endm

.macro pop ra, rb
ldp \ra, \rb, [sp], #16
.endm

.macro tbzmask, reg, mask, label, shift=0
.if \shift >= 64
    .error "tbzmask: unsupported mask, \mask"
.elseif \mask == 1 << \shift
    tbz     \reg, #\shift, \label
.else
    tbzmask \reg, \mask, \label, "(\shift + 1)"
.endif
.endm

.macro tbnzmask, reg, mask, label, shift=0
.if \shift >= 64
    .error "tbnzmask: unsupported mask, \mask"
.elseif \mask == 1 << \shift
    tbnz     \reg, #\shift, \label
.else
    tbnzmask \reg, \mask, \label, "(\shift + 1)"
.endif
.endm

.macro calloc_bootmem_aligned, new_ptr, new_ptr_end, tmp, size_shift, phys_offset
.if \size_shift < 4
    .error "calloc_bootmem_aligned: Unsupported size_shift, \size_shift"
.endif

    /* load boot_alloc_end */
    adrp    \tmp, boot_alloc_end
    ldr     \new_ptr, [\tmp, #:lo12:boot_alloc_end]

    /* align to page */
.if \size_shift > 12
    add     \new_ptr, \new_ptr, #(1 << \size_shift)
    sub     \new_ptr, \new_ptr, #1
.else
    add     \new_ptr, \new_ptr, #(1 << \size_shift) - 1
.endif
    and     \new_ptr, \new_ptr, #~((1 << \size_shift) - 1)

    /* add one page and store boot_alloc_end */
    add     \new_ptr_end, \new_ptr, #(1 << \size_shift)
    str     \new_ptr_end, [\tmp, #:lo12:boot_alloc_end]

    /* clean and invalidate boot_alloc_end pointer */
    add     x0, \tmp, #:lo12:boot_alloc_end
    mov     x1, #8
    bl      arch_clean_invalidate_cache_range

    /* calculate virtual address */
    sub     \new_ptr, \new_ptr, \phys_offset
    sub     \new_ptr_end, \new_ptr_end, \phys_offset

    /* clean and invalidate new page */
    mov     x0, \new_ptr
    mov     x1, #(1 << \size_shift)
    bl      arch_clean_invalidate_cache_range

    /* clear page */
    mov     \tmp, \new_ptr
.Lcalloc_bootmem_aligned_clear_loop\@:
    stp     xzr, xzr, [\tmp], #16
    cmp     \tmp, \new_ptr_end
    b.lo    .Lcalloc_bootmem_aligned_clear_loop\@
.endm

/* Set fault handler for next instruction */
.macro set_fault_handler, handler
.Lfault_location\@:
.pushsection .rodata.fault_handler_table
    .quad    .Lfault_location\@
    .quad    \handler
.popsection
.endm

/*
    * Helper macro to generate the best mov/movk combinations according
    * the value to be moved. The 16 bits from '_shift' are tested and
    * if not zero, they are moved into '_reg' without affecting
    * other bits.
    */
.macro _mov_imm16 _reg, _val, _shift
    .if (\_val >> \_shift) & 0xffff
        .if (\_val & (1 << \_shift - 1))
            movk	\_reg, (\_val >> \_shift) & 0xffff, LSL \_shift
        .else
            mov	\_reg, \_val & (0xffff << \_shift)
        .endif
    .endif
.endm

/*
    * Helper macro to load arbitrary values into 32 or 64-bit registers
    * which generates the best mov/movk combinations. Many base addresses
    * are 64KB aligned the macro will eliminate updating bits 15:0 in
    * that case
    */
.macro mov_imm _reg, _val
    .if (\_val) == 0
        mov	\_reg, #0
    .else
        _mov_imm16	\_reg, (\_val), 0
        _mov_imm16	\_reg, (\_val), 16
        _mov_imm16	\_reg, (\_val), 32
        _mov_imm16	\_reg, (\_val), 48
    .endif
.endm

	/*
	 * Macro to mark instances where we're jumping to a function and don't
	 * expect a return. To provide the function being jumped to with
	 * additional information, we use 'bl' instruction to jump rather than
	 * 'b'.
         *
	 * Debuggers infer the location of a call from where LR points to, which
	 * is usually the instruction after 'bl'. If this macro expansion
	 * happens to be the last location in a function, that'll cause the LR
	 * to point a location beyond the function, thereby misleading debugger
	 * back trace. We therefore insert a 'nop' after the function call for
	 * debug builds, unless 'skip_nop' parameter is non-zero.
	 */
	.macro no_ret _func:req, skip_nop=0
	bl	\_func
#if DEBUG
	.ifeq \skip_nop
	nop
	.endif
#endif
	.endm