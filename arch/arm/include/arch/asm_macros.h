/*
 * Copyright (c) 2015, Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/* Set fault handler for next instruction */
.macro set_fault_handler, handler
.Lfault_location\@:
.pushsection .rodata.fault_handler_table
.long    .Lfault_location\@
.long    \handler
.popsection
.endm

/*
 * Helper macro to generate the best mov/movw/movt combinations
 * according to the value to be moved.
 */
.macro mov_imm _reg, _val
        .if ((\_val) & 0xffff0000) == 0
                mov	\_reg, #(\_val)
        .else
                movw	\_reg, #((\_val) & 0xffff)
                movt	\_reg, #((\_val) >> 16)
        .endif
.endm

.macro calloc_bootmem_aligned, new_ptr, new_ptr_end, tmp, size_shift, phys_offset
.if \size_shift < 4
    .error "calloc_bootmem_aligned: Unsupported size_shift, \size_shift"
.endif

    /* load boot_alloc_end */
    ldr    \tmp, =boot_alloc_end
    ldr     \new_ptr, [\tmp]

    ldr     \tmp, =(1 << \size_shift)
    /* align to page */
.if \size_shift > 12
    add     \new_ptr, \new_ptr, \tmp
    sub     \new_ptr, \new_ptr, #1
    sub     \tmp, \tmp, #1
.else
    sub     \tmp, \tmp, #1
    add     \new_ptr, \new_ptr, \tmp
.endif

    mvn     \tmp, \tmp
    and     \new_ptr, \new_ptr, \tmp

    /* add one page and store boot_alloc_end */
    ldr     \tmp, =(1 << \size_shift)
    add     \new_ptr_end, \new_ptr, \tmp
    ldr     \tmp, =boot_alloc_end
    str     \new_ptr_end, [\tmp]

    /* clean and invalidate boot_alloc_end pointer */
    mov     r0, \tmp
    mov     r1, #4
    bl      arch_clean_invalidate_cache_range

    /* calculate virtual address */
    sub     \new_ptr, \new_ptr, \phys_offset
    sub     \new_ptr_end, \new_ptr_end, \phys_offset

    /* clean and invalidate new page */
    mov     r0, \new_ptr
    ldr     \tmp, =(1 << \size_shift)
    mov     r1, \tmp
    bl      arch_clean_invalidate_cache_range

    /* clear page */
    mov     r0, #0
    mov     \tmp, \new_ptr
.Lcalloc_bootmem_aligned_clear_loop\@:
    str     r0, [\tmp], #4
    cmp     \tmp, \new_ptr_end
    blo    .Lcalloc_bootmem_aligned_clear_loop\@
.endm
