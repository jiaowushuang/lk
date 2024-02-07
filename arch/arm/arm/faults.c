/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kern/debug.h>
#include <kern/bits.h>
#include <arch/arm.h>
#include <kernel/thread.h>
#include <platform.h>
#include <arch.h>

struct fault_handler_table_entry {
    uint32_t pc;
    uint32_t fault_handler;
};

extern struct fault_handler_table_entry __fault_handler_table_start[];
extern struct fault_handler_table_entry __fault_handler_table_end[];
extern void do_trap_guest_sync(struct arm_fault_frame *regs);
extern void do_trap_hyp_serror(struct arm_fault_frame *regs);
extern void do_trap_guest_serror(struct arm_fault_frame *regs);

static void dump_mode_regs(uint32_t spsr, uint32_t svc_r13, uint32_t svc_r14) {
    struct arm_mode_regs regs;
    arm_save_mode_regs(&regs);

    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_USR) ? '*' : ' ', "usr", regs.usr_r13, regs.usr_r14);
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_FIQ) ? '*' : ' ', "fiq", regs.fiq_r13, regs.fiq_r14);
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_IRQ) ? '*' : ' ', "irq", regs.irq_r13, regs.irq_r14);
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", 'a', "svc", regs.svc_r13, regs.svc_r14);
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_SVC) ? '*' : ' ', "svc", svc_r13, svc_r14);
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_UND) ? '*' : ' ', "und", regs.und_r13, regs.und_r14);
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_SYS) ? '*' : ' ', "sys", regs.sys_r13, regs.sys_r14);
#ifdef WITH_HYPER_MODE    
    dprintf(CRITICAL, "%c%s r13 0x%08x r14 0x%08x\n", ((spsr & CPSR_MODE_MASK) == CPSR_MODE_HYP) ? '*' : ' ', "sys", regs.hyp_r13, regs.hyp_r14);
#endif
    // dump the bottom of the current stack
    addr_t stack;
    switch (spsr & CPSR_MODE_MASK) {
        case CPSR_MODE_FIQ:
            stack = regs.fiq_r13;
            break;
        case CPSR_MODE_IRQ:
            stack = regs.irq_r13;
            break;
        case CPSR_MODE_SVC:
            stack = svc_r13;
            break;
        case CPSR_MODE_UND:
            stack = regs.und_r13;
            break;
        case CPSR_MODE_SYS:
            stack = regs.sys_r13;
            break;
#ifdef WITH_HYPER_MODE
        case CPSR_MODE_HYP:
		    stack = regs.hyp_r13;
		    break;
#endif
	    default:
            stack = 0;
    }

    if (stack != 0) {
        dprintf(CRITICAL, "bottom of stack at 0x%08x:\n", (unsigned int)stack);
        hexdump((void *)stack, 128);
    }
}

static void dump_fault_frame(struct arm_fault_frame *frame) {
    struct thread *current_thread = get_current_thread();

    dprintf(CRITICAL, "current_thread %p, name %s\n",
            current_thread, current_thread ? current_thread->name : "");

    dprintf(CRITICAL, "r0  0x%08x r1  0x%08x r2  0x%08x r3  0x%08x\n", frame->r[0], frame->r[1], frame->r[2], frame->r[3]);
    dprintf(CRITICAL, "r4  0x%08x r5  0x%08x r6  0x%08x r7  0x%08x\n", frame->r[4], frame->r[5], frame->r[6], frame->r[7]);
    dprintf(CRITICAL, "r8  0x%08x r9  0x%08x r10 0x%08x r11 0x%08x\n", frame->r[8], frame->r[9], frame->r[10], frame->r[11]);
    dprintf(CRITICAL, "r12 0x%08x usp 0x%08x ulr 0x%08x pc  0x%08x\n", frame->r[12], frame->usp, frame->ulr, frame->pc);
    dprintf(CRITICAL, "spsr 0x%08x\n", frame->spsr);

    dump_mode_regs(frame->spsr, (uintptr_t)(frame + 1), frame->lr);
}

static void dump_iframe(struct arm_iframe *frame) {
    dprintf(CRITICAL, "r0  0x%08x r1  0x%08x r2  0x%08x r3  0x%08x\n", frame->r0, frame->r1, frame->r2, frame->r3);
    dprintf(CRITICAL, "r12 0x%08x usp 0x%08x ulr 0x%08x pc  0x%08x\n", frame->r12, frame->usp, frame->ulr, frame->pc);
    dprintf(CRITICAL, "spsr 0x%08x\n", frame->spsr);

    dump_mode_regs(frame->spsr, (uintptr_t)(frame + 1), frame->lr);
}

static void exception_die(struct arm_fault_frame *frame, const char *msg) {
    dprintf(CRITICAL, msg);
    dump_fault_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    for (;;);
}

static void exception_die_iframe(struct arm_iframe *frame, const char *msg) {
    dprintf(CRITICAL, msg);
    dump_iframe(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    for (;;);
}

void arm_syscall_handler(struct arm_fault_frame *frame);
__WEAK void arm_syscall_handler(struct arm_fault_frame *frame) {
    exception_die(frame, "unhandled syscall, halting\n");
}

void arm_undefined_handler(struct arm_iframe *frame);
void arm_undefined_handler(struct arm_iframe *frame) {
    /* look at the undefined instruction, figure out if it's something we can handle */
    bool in_thumb = frame->spsr & (1<<5);
    if (in_thumb) {
        frame->pc -= 2;
    } else {
        frame->pc -= 4;
    }

    __UNUSED uint32_t opcode = *(uint32_t *)frame->pc;
    //dprintf(CRITICAL, "undefined opcode 0x%x\n", opcode);

#if ARM_WITH_VFP
    if (in_thumb) {
        /* look for a 32bit thumb instruction */
        if (opcode & 0x0000e800) {
            /* swap the 16bit words */
            opcode = (opcode >> 16) | (opcode << 16);
        }

        if (((opcode & 0xec000e00) == 0xec000a00) || // vfp
                ((opcode & 0xef000000) == 0xef000000) || // advanced simd data processing
                ((opcode & 0xff100000) == 0xf9000000)) { // VLD

            //dprintf(CRITICAL, "vfp/neon thumb instruction 0x%08x at 0x%x\n", opcode, frame->pc);
            goto fpu;
        }
    } else {
        /* look for arm vfp/neon coprocessor instructions */
        if (((opcode & 0x0c000e00) == 0x0c000a00) || // vfp
                ((opcode & 0xfe000000) == 0xf2000000) || // advanced simd data processing
                ((opcode & 0xff100000) == 0xf4000000)) { // VLD
            //dprintf(CRITICAL, "vfp/neon arm instruction 0x%08x at 0x%x\n", opcode, frame->pc);
            goto fpu;
        }
    }
#endif

    exception_die_iframe(frame, "undefined abort, halting\n");
    return;

#if ARM_WITH_VFP
fpu:
    arm_fpu_undefined_instruction(frame);
#endif
}

void arm_data_abort_handler(struct arm_fault_frame *frame);
void arm_data_abort_handler(struct arm_fault_frame *frame) { /* diff svc mode or usr mode */
    struct fault_handler_table_entry *fault_handler;
    uint32_t fsr = arm_read_dfsr();
    uint32_t far = arm_read_dfar();
    uint32_t abt_level = frame->spsr & 0xf;

    dprintf(CRITICAL, "data abort happen at %s level\n", abt_level ? "svc mode" : "usr mode");
    for (fault_handler = __fault_handler_table_start; fault_handler < __fault_handler_table_end; fault_handler++) {
        if (fault_handler->pc == frame->pc) {
            frame->pc = fault_handler->fault_handler;
            return;
        }
    }

    dprintf(CRITICAL, "\n\ncpu %u data abort, ", arch_curr_cpu_num());
    bool write = !!BIT(fsr, 11);

#if defined(ARM_FSR_LONG_FORMAT)
     uint32_t fault_status = BITS(fsr, 5, 0);
    /* decode the fault status (from table B3-24) */
    switch (fault_status) {
        case 0b000100: // translation fault
            dprintf(CRITICAL, "MPU translation fault on %s\n", write ? "write" : "read");
            break;
        case 0b000101: 
            dprintf(CRITICAL, "MMU translation level1 fault on %s\n", write ? "write" : "read");
            break;
        case 0b000110:
            dprintf(CRITICAL, "MMU translation level2 fault on %s\n", write ? "write" : "read");
            break;
        case 0b000111:
            dprintf(CRITICAL, "MMU translation level3 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001001: // access flag fault
            dprintf(CRITICAL, "access flaglevel1 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001010: 
            dprintf(CRITICAL, "access flag level2 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001011:
            dprintf(CRITICAL, "access flag level3 fault on %s\n", write ? "write" : "read");
            break;    
        case 0b001100: // permission fault
            dprintf(CRITICAL, "MPU permission fault on %s\n", write ? "write" : "read");
            break;
        case 0b001101:
            dprintf(CRITICAL, "MMU permission level1 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001110:
            dprintf(CRITICAL, "MMU permission level2 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001111:
            dprintf(CRITICAL, "MMU permission level3 fault on %s\n", write ? "write" : "read");
            break;
        case 0b010000: // Synchronous external abort
            dprintf(CRITICAL, "Synchronous external abort on %s\n", write ? "write" : "read");
            break;
        case 0b010001: // Asynchronous external abort/SError interrupt
            dprintf(CRITICAL, "Asynchronous external abort/SError interrupt on %s\n", write ? "write" : "read");
            break;
        case 0b010101: // Synchronous external abort on translation table walk
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 1 on %s\n", write ? "write" : "read");
            break;
        case 0b010110:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 2 on %s\n", write ? "write" : "read");
            break;
        case 0b010111:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 3 on %s\n", write ? "write" : "read");
            break;
        case 0b011000: // Synchronous parity error/or ECC error on memory access
            dprintf(CRITICAL, "Synchronous parity error/or ECC error on memory access on %s\n", write ? "write" : "read");
		    break;
        case 0b011001: // Asynchronous parity error on memory access
            dprintf(CRITICAL, "Asynchronous parity error on memory access on %s\n", write ? "write" : "read");
            break;
        case 0b011101: // Synchronous parity error on memory access on translation table walk
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 1 on %s\n", write ? "write" : "read");
            break;
        case 0b011110:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 2 on %s\n", write ? "write" : "read");
            break;
        case 0b011111:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 3 on %s\n", write ? "write" : "read");
            break;
        case 0b100001: // Alignment fault
            dprintf(CRITICAL, "Alignment fault on %s\n", write ? "write" : "read");
            break;
        case 0b100010: // Debug event
            dprintf(CRITICAL, "Debug event on %s\n", write ? "write" : "read");
            break;
        case 0b110000: // TLB conflict abort
            dprintf(CRITICAL, "TLB conflict abort on %s\n", write ? "write" : "read");
            break;
        case 0b111101: // Domain fault
            dprintf(CRITICAL, "Domain fault 1 on %s\n", write ? "write" : "read");
            break;
        case 0b111110:
            dprintf(CRITICAL, "Domain fault 2 on %s\n", write ? "write" : "read");
            break;
        case 0b111111:
            dprintf(CRITICAL, "Domain fault 3 on %s\n", write ? "write" : "read");
            break;
	    default:
            dprintf(CRITICAL, "unhandled fault\n");
            ;
    }
#else /* ARM_FSR_SHORT_FORMAT */
    uint32_t fault_status = (BIT(fsr, 10) ? (1<<4) : 0) |  BITS(fsr, 3, 0);

    /* decode the fault status (from table B3-23) */
    switch (fault_status) {
        case 0b00001: // alignment fault
            dprintf(CRITICAL, "alignment fault on %s\n", write ? "write" : "read");
            break;
        case 0b00101:
        case 0b00111: // translation fault
            dprintf(CRITICAL, "translation fault on %s\n", write ? "write" : "read");
            break;
        case 0b00011:
        case 0b00110: // access flag fault
            dprintf(CRITICAL, "access flag fault on %s\n", write ? "write" : "read");
            break;
        case 0b01001:
        case 0b01011: // domain fault
            dprintf(CRITICAL, "domain fault, domain %lu\n", BITS_SHIFT(fsr, 7, 4));
            break;
        case 0b01101:
        case 0b01111: // permission fault
            dprintf(CRITICAL, "permission fault on %s\n", write ? "write" : "read");
            break;
        case 0b00010: // debug event
            dprintf(CRITICAL, "debug event\n");
            break;
        case 0b01000: // synchronous external abort
            dprintf(CRITICAL, "synchronous external abort on %s\n", write ? "write" : "read");
            break;
        case 0b10110: // asynchronous external abort
            dprintf(CRITICAL, "asynchronous external abort on %s\n", write ? "write" : "read");
            break;
        case 0b10000: // TLB conflict event
        case 0b11001: // synchronous parity error on memory access
        case 0b00100: // fault on instruction cache maintenance
        case 0b01100: // synchronous external abort on translation table walk
        case 0b01110: //    "
        case 0b11100: // synchronous parity error on translation table walk
        case 0b11110: //    "
        case 0b11000: // asynchronous parity error on memory access
        default:
            dprintf(CRITICAL, "unhandled fault\n");
            ;
    }
#endif /* ARM_FSR_LONG_FORMAT */
    dprintf(CRITICAL, "DFAR 0x%x (fault address)\n", far);
    dprintf(CRITICAL, "DFSR 0x%x (fault status register)\n", fsr);

    exception_die(frame, "halting\n");
}

void arm_prefetch_abort_handler(struct arm_fault_frame *frame);
void arm_prefetch_abort_handler(struct arm_fault_frame *frame) { /* diff svc mode or usr mode */
    uint32_t fsr = arm_read_ifsr();
    uint32_t far = arm_read_ifar();
    uint32_t abt_level = frame->spsr & 0xf;

    dprintf(CRITICAL, "data abort happen at %s level\n", abt_level ? "svc mode" : "usr mode");
    dprintf(CRITICAL, "\n\ncpu %u prefetch abort, ", arch_curr_cpu_num());

#if defined(ARM_FSR_LONG_FORMAT)
    uint32_t fault_status = BITS(fsr, 5, 0);
    /* decode the fault status (from table B3-24) */
    switch (fault_status) {
        case 0b000100: // translation fault
            dprintf(CRITICAL, "MPU translation fault\n");
            break;
        case 0b000101: 
            dprintf(CRITICAL, "MMU translation level1 fault\n");
            break;
        case 0b000110:
            dprintf(CRITICAL, "MMU translation level2 fault\n");
            break;
        case 0b000111:
            dprintf(CRITICAL, "MMU translation level3 fault\n");
            break;
        case 0b001001: // access flag fault
            dprintf(CRITICAL, "access flaglevel1 fault\n");
            break;
        case 0b001010: 
            dprintf(CRITICAL, "access flag level2 fault\n");
            break;
        case 0b001011:
            dprintf(CRITICAL, "access flag level3 fault\n");
            break;    
        case 0b001100: // permission fault
            dprintf(CRITICAL, "MPU permission fault\n");
            break;
        case 0b001101:
            dprintf(CRITICAL, "MMU permission level1 fault\n");
            break;
        case 0b001110:
            dprintf(CRITICAL, "MMU permission level2 fault\n");
            break;
        case 0b001111:
            dprintf(CRITICAL, "MMU permission level3 fault\n");
            break;
        case 0b010000: // Synchronous external abort
            dprintf(CRITICAL, "Synchronous external abort\n");
            break;
        case 0b010001: // Asynchronous external abort
            dprintf(CRITICAL, "Asynchronous external abort\n");
            break;
        case 0b010101: // Synchronous external abort on translation table walk
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 1\n");
            break;
        case 0b010110:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 2\n");
            break;
        case 0b010111:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 3\n");
            break;
        case 0b011000: // Synchronous parity error/or ECC error on memory access
            dprintf(CRITICAL, "Synchronous parity error/or ECC error on memory access\n");
		    break;
        case 0b011001: // Asynchronous parity error on memory access
            dprintf(CRITICAL, "Asynchronous parity error on memory access\n");
            break;
        case 0b011101: // Synchronous parity error on memory access on translation table walk
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 1\n");
            break;
        case 0b011110:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 2\n");
            break;
        case 0b011111:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 3\n");
            break;
        case 0b100001: // Alignment fault
            dprintf(CRITICAL, "Alignment fault\n");
            break;
        case 0b100010: // Debug event
            dprintf(CRITICAL, "Debug event\n");
            break;
        case 0b110000: // TLB conflict abort
            dprintf(CRITICAL, "TLB conflict abort\n");
            break;
        case 0b111101: // Domain fault
            dprintf(CRITICAL, "Domain fault 1\n");
            break;
        case 0b111110:
            dprintf(CRITICAL, "Domain fault 2\n");
            break;
        case 0b111111:
            dprintf(CRITICAL, "Domain fault 3\n");
            break;
	    default:
            dprintf(CRITICAL, "unhandled fault\n");
            ;
    }
#else /* ARM_FSR_SHORT_FORMAT */
    uint32_t fault_status = (BIT(fsr, 10) ? (1<<4) : 0) |  BITS(fsr, 3, 0);

    /* decode the fault status (from table B3-23) */
    switch (fault_status) {
        case 0b00001: // alignment fault
            dprintf(CRITICAL, "alignment fault\n");
            break;
        case 0b00101:
        case 0b00111: // translation fault
            dprintf(CRITICAL, "translation fault\n");
            break;
        case 0b00011:
        case 0b00110: // access flag fault
            dprintf(CRITICAL, "access flag fault\n");
            break;
        case 0b01001:
        case 0b01011: // domain fault
            dprintf(CRITICAL, "domain fault, domain %lu\n", BITS_SHIFT(fsr, 7, 4));
            break;
        case 0b01101:
        case 0b01111: // permission fault
            dprintf(CRITICAL, "permission fault\n");
            break;
        case 0b00010: // debug event
            dprintf(CRITICAL, "debug event\n");
            break;
        case 0b01000: // synchronous external abort
            dprintf(CRITICAL, "synchronous external abort\n");
            break;
        case 0b10110: // asynchronous external abort
            dprintf(CRITICAL, "asynchronous external abort\n");
            break;
        case 0b10000: // TLB conflict event
        case 0b11001: // synchronous parity error on memory access
        case 0b00100: // fault on instruction cache maintenance
        case 0b01100: // synchronous external abort on translation table walk
        case 0b01110: //    "
        case 0b11100: // synchronous parity error on translation table walk
        case 0b11110: //    "
        case 0b11000: // asynchronous parity error on memory access
        default:
            dprintf(CRITICAL, "unhandled fault\n");
            ;
    }
#endif /* ARM_FSR_LONG_FORMAT */
    dprintf(CRITICAL, "IFAR 0x%x (fault address)\n", far);
    dprintf(CRITICAL, "IFSR 0x%x (fault status register)\n", fsr);

    exception_die(frame, "halting\n");
}

#ifdef WITH_HYPER_MODE

void arm_vsyscall_handler(struct arm_fault_frame *frame, uint32_t hsr);
void arm_vsyscall_handler(struct arm_fault_frame *frame, uint32_t hsr)
{
	uint32_t fault_ec, fault_il;
 
	fault_ec = (hsr & HSREC_MASK) >> HSREC_SHIFT;
	fault_il = hsr & HSRIL32;

	DEBUG_ASSERT(fault_ec == 0x11 || fault_ec == 0x12);
	dprintf(CRITICAL, "Trap by %s instruction\n", fault_il ? "32-bits" : "16-bits");

	uint32_t far = read_hifar();
	dprintf(CRITICAL, "HIFAR 0x%x (fault address)\n", far);
	dprintf(CRITICAL, "HSR 0x%x (fault status register)\n", hsr);
    
#ifdef WITH_SYSCALL_TABLE
    extern const unsigned long syscall_table[];

    uint16_t fault_iss;

	fault_iss = (uint16_t)(hsr & (~(HSREC_MASK | HSRIL32)));

	unsigned long sys_fn = syscall_table[fault_iss];
    dprintf(CRITICAL, "syscall handler %lx, syscall number %d\n", sys_fn, fault_iss);
#endif

    exception_die(frame, "unhandled syscall, halting\n");
}

void arm_vundefined_handler(struct arm_fault_frame *frame, uint32_t hsr);
void arm_vundefined_handler(struct arm_fault_frame *frame, uint32_t hsr)
{
	exception_die(frame, "undefined abort, halting\n");
	return;
}

void arm_vdata_abort_handler(struct arm_fault_frame *frame, uint32_t hsr);
void arm_vdata_abort_handler(struct arm_fault_frame *frame, uint32_t hsr) /* diff hyp mode or other mode */
{
	uint32_t fault_ec, fault_il, fault_iss;

	fault_ec = (hsr & HSREC_MASK) >> HSREC_SHIFT;
	fault_il = hsr & HSRIL32;
	fault_iss = hsr & (~(HSREC_MASK | HSRIL32));

	DEBUG_ASSERT(fault_ec == 0x24 || fault_ec == 0x25);
    dprintf(CRITICAL, "Data abort happen at %s level\n", fault_ec==0x24 ? "PL0/1 mode" : "PL2 mode");
	dprintf(CRITICAL, "Trap by %s instruction\n", fault_il ? "32-bits" : "16-bits");

	uint32_t far = read_hdfar();
	dprintf(CRITICAL, "HDFAR 0x%x (fault address)\n", far);

    uint32_t hpfar = read_hpfar();
    dprintf(CRITICAL, "HPFAR 0x%x (fault address of IPA[39:12])\n", hpfar >> 4);
   
	dprintf(CRITICAL, "HSR 0x%x (fault status register)\n", hsr);
    dprintf(CRITICAL, "Data abort EA:%s, CM:%s, S1PTW:%s\n", 
            BIT(fault_iss, 9) ? "yes" : "no", 
            BIT(fault_iss, 8) ? "yes" : "no", 
            BIT(fault_iss, 7) ? "yes" : "no");
    
    uint32_t write, fault_status;

    write = BIT(fault_iss, 6);
    fault_status = fault_iss & 0x3f;

    /* decode the fault status (from table B3-24) */
    switch (fault_status) {
        case 0b000100: // translation fault
            dprintf(CRITICAL, "MPU translation fault on %s\n", write ? "write" : "read");
            break;
        case 0b000101: 
            dprintf(CRITICAL, "MMU translation level1 fault on %s\n", write ? "write" : "read");
            break;
        case 0b000110:
            dprintf(CRITICAL, "MMU translation level2 fault on %s\n", write ? "write" : "read");
            break;
        case 0b000111:
            dprintf(CRITICAL, "MMU translation level3 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001001: // access flag fault
            dprintf(CRITICAL, "access flaglevel1 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001010: 
            dprintf(CRITICAL, "access flag level2 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001011:
            dprintf(CRITICAL, "access flag level3 fault on %s\n", write ? "write" : "read");
            break;    
        case 0b001100: // permission fault
            dprintf(CRITICAL, "MPU permission fault on %s\n", write ? "write" : "read");
            break;
        case 0b001101:
            dprintf(CRITICAL, "MMU permission level1 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001110:
            dprintf(CRITICAL, "MMU permission level2 fault on %s\n", write ? "write" : "read");
            break;
        case 0b001111:
            dprintf(CRITICAL, "MMU permission level3 fault on %s\n", write ? "write" : "read");
            break;
        case 0b010000: // Synchronous external abort
            dprintf(CRITICAL, "Synchronous external abort on %s\n", write ? "write" : "read");
            break;
        case 0b010001: // Asynchronous external abort/SError interrupt
            dprintf(CRITICAL, "Asynchronous external abort/SError interrupt on %s\n", write ? "write" : "read");
            break;
        case 0b010101: // Synchronous external abort on translation table walk
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 1 on %s\n", write ? "write" : "read");
            break;
        case 0b010110:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 2 on %s\n", write ? "write" : "read");
            break;
        case 0b010111:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 3 on %s\n", write ? "write" : "read");
            break;
        case 0b011000: // Synchronous parity error/or ECC error on memory access
            dprintf(CRITICAL, "Synchronous parity error/or ECC error on memory access on %s\n", write ? "write" : "read");
		    break;
        case 0b011001: // Asynchronous parity error on memory access
            dprintf(CRITICAL, "Asynchronous parity error on memory access on %s\n", write ? "write" : "read");
            break;
        case 0b011101: // Synchronous parity error on memory access on translation table walk
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 1 on %s\n", write ? "write" : "read");
            break;
        case 0b011110:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 2 on %s\n", write ? "write" : "read");
            break;
        case 0b011111:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 3 on %s\n", write ? "write" : "read");
            break;
        case 0b100001: // Alignment fault
            dprintf(CRITICAL, "Alignment fault on %s\n", write ? "write" : "read");
            break;
        case 0b100010: // Debug event
            dprintf(CRITICAL, "Debug event on %s\n", write ? "write" : "read");
            break;
        case 0b110000: // TLB conflict abort
            dprintf(CRITICAL, "TLB conflict abort on %s\n", write ? "write" : "read");
            break;
        case 0b111101: // Domain fault
            dprintf(CRITICAL, "Domain fault 1 on %s\n", write ? "write" : "read");
            break;
        case 0b111110:
            dprintf(CRITICAL, "Domain fault 2 on %s\n", write ? "write" : "read");
            break;
        case 0b111111:
            dprintf(CRITICAL, "Domain fault 3 on %s\n", write ? "write" : "read");
            break;
	    default:
            dprintf(CRITICAL, "unhandled fault\n");
            ;
    }
	exception_die(frame, "halting\n");
}

void arm_vprefetch_abort_handler(struct arm_fault_frame *frame, uint32_t hsr);
void arm_vprefetch_abort_handler(struct arm_fault_frame *frame, uint32_t hsr) /* diff hyp mode or other mode */
{
	uint32_t fault_ec, fault_il, fault_iss;

	fault_ec = (hsr & HSREC_MASK) >> HSREC_SHIFT;
	fault_il = hsr & HSRIL32;
	fault_iss = hsr & (~(HSREC_MASK | HSRIL32));
	DEBUG_ASSERT(fault_ec == 0x20 || fault_ec == 0x21);
    dprintf(CRITICAL, "Prefetch abort happen at %s level\n", fault_ec==0x20 ? "PL0/1 mode" : "PL2 mode");
	dprintf(CRITICAL, "Trap by %s instruction\n", fault_il ? "32-bits" : "16-bits");
	uint32_t far = read_hifar();
	dprintf(CRITICAL, "HIFAR 0x%x (fault address)\n", far);

    uint32_t hpfar = read_hpfar();
    dprintf(CRITICAL, "HPFAR 0x%x (fault address of IPA[39:12])\n", hpfar >> 4);
 
	dprintf(CRITICAL, "HSR 0x%x (fault status register)\n", hsr);
    dprintf(CRITICAL, "Prefetch abort EA:%s, S1PTW:%s\n", 
            BIT(fault_iss, 9) ? "yes" : "no", 
            BIT(fault_iss, 7) ? "yes" : "no");
    
    uint32_t fault_status;

    fault_status = fault_iss & 0x3f;

    /* decode the fault status (from table B3-24) */
    switch (fault_status) {
        case 0b000100: // translation fault
            dprintf(CRITICAL, "MPU translation fault\n");
            break;
        case 0b000101: 
            dprintf(CRITICAL, "MMU translation level1 fault\n");
            break;
        case 0b000110:
            dprintf(CRITICAL, "MMU translation level2 fault\n");
            break;
        case 0b000111:
            dprintf(CRITICAL, "MMU translation level3 fault\n");
            break;
        case 0b001001: // access flag fault
            dprintf(CRITICAL, "access flaglevel1 fault\n");
            break;
        case 0b001010: 
            dprintf(CRITICAL, "access flag level2 fault\n");
            break;
        case 0b001011:
            dprintf(CRITICAL, "access flag level3 fault\n");
            break;    
        case 0b001100: // permission fault
            dprintf(CRITICAL, "MPU permission fault\n");
            break;
        case 0b001101:
            dprintf(CRITICAL, "MMU permission level1 fault\n");
            break;
        case 0b001110:
            dprintf(CRITICAL, "MMU permission level2 fault\n");
            break;
        case 0b001111:
            dprintf(CRITICAL, "MMU permission level3 fault\n");
            break;
        case 0b010000: // Synchronous external abort
            dprintf(CRITICAL, "Synchronous external abort\n");
            break;
        case 0b010001: // Asynchronous external abort
            dprintf(CRITICAL, "Asynchronous external abort\n");
            break;
        case 0b010101: // Synchronous external abort on translation table walk
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 1\n");
            break;
        case 0b010110:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 2\n");
            break;
        case 0b010111:
            dprintf(CRITICAL, "Synchronous external abort on translation table walk 3\n");
            break;
        case 0b011000: // Synchronous parity error/or ECC error on memory access
            dprintf(CRITICAL, "Synchronous parity error/or ECC error on memory access\n");
		    break;
        case 0b011001: // Asynchronous parity error on memory access
            dprintf(CRITICAL, "Asynchronous parity error on memory access\n");
            break;
        case 0b011101: // Synchronous parity error on memory access on translation table walk
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 1\n");
            break;
        case 0b011110:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 2\n");
            break;
        case 0b011111:
            dprintf(CRITICAL, "Synchronous parity error on memory access on translation table walk 3\n");
            break;
        case 0b100001: // Alignment fault
            dprintf(CRITICAL, "Alignment fault\n");
            break;
        case 0b100010: // Debug event
            dprintf(CRITICAL, "Debug event\n");
            break;
        case 0b110000: // TLB conflict abort
            dprintf(CRITICAL, "TLB conflict abort\n");
            break;
        case 0b111101: // Domain fault
            dprintf(CRITICAL, "Domain fault 1\n");
            break;
        case 0b111110:
            dprintf(CRITICAL, "Domain fault 2\n");
            break;
        case 0b111111:
            dprintf(CRITICAL, "Domain fault 3\n");
            break;
	    default:
            dprintf(CRITICAL, "unhandled fault\n");
            ;
    }
	exception_die(frame, "halting\n");
}

void arm_vcpu_handler(struct arm_fault_frame *frame, uint32_t hsr);
void arm_vcpu_handler(struct arm_fault_frame *frame, uint32_t hsr)
{
	uint32_t fault_ec, fault_il, fault_iss;
	uint32_t far = read_hifar();

	fault_ec = (hsr & HSREC_MASK) >> HSREC_SHIFT;
	fault_il = hsr & HSRIL32;
	fault_iss = hsr & (~(HSREC_MASK | HSRIL32));

	dprintf(CRITICAL, "Trap by %s instruction\n", fault_il ? "32-bits" : "16-bits");
    dprintf(CRITICAL, "vcpu ec - %x, iss - %x\n", fault_ec, fault_iss);
	dprintf(CRITICAL, "HIFAR 0x%x (fault address)\n", far);
	dprintf(CRITICAL, "HSR 0x%x (fault status register)\n", hsr);

    frame->hsr = hsr;
    do_trap_guest_sync(frame);
}
#endif /* WITH_HYPER_MODE */