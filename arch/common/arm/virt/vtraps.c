/*
 * xen/arch/arm/traps.c
 *
 * ARM Trap handlers
 *
 * Copyright (c) 2011 Citrix Systems.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/arch_helpers.h>
#include <arch/arm.h>
#include <arch/sysregs.h>
#include <arch/vcpu.h>
#include <asm/vtraps.h>
#include <assert.h>
#include <kernel/debug.h>
#include <stdbool.h>
#include <stdint.h>
#include <kernel/thread.h>
#include <errno.h>

#ifndef IS_64BIT
#define ARCH_BYTES_BITS 4
#else
#define ARCH_BYTES_BITS 8
#endif

static int is_32bit_domain = 1;

static enum {
  TRAP,
  NATIVE,
} vwfi;

u_register_t get_default_hcr_flags(void) {
  return (HCR_PTW | HCR_BSU(1) | HCR_AMO | HCR_IMO | HCR_FMO | HCR_VM |
          (vwfi != NATIVE ? (HCR_TWI | HCR_TWE) : 0) | HCR_TID3 | HCR_TSC |
          HCR_TAC | HCR_SWIO | HCR_TIDCP | HCR_FB | HCR_TSW);
}

void init_traps(void) {
  /* Trap Debug and Performance Monitor accesses */
  WRITE_SYSREG(HDCR_TDRA | HDCR_TDOSA | HDCR_TDA | HDCR_TPM | HDCR_TPMCR,
               MDCR_EL2);

  /* Trap CP15 c15 used for implementation defined registers */
  WRITE_SYSREG(HSTR_T(15), HSTR_EL2);

  /* Trap all coprocessor registers (0-13) except cp10 and
   * cp11 for VFP.
   *
   * /!\ All coprocessors except cp10 and cp11 cannot be used in Xen.
   *
   * On ARM64 the TCPx bits which we set here (0..9,12,13) are all
   * RES1, i.e. they would trap whether we did this write or not.
   */
  WRITE_SYSREG(
      (HCPTR_CP_MASK & ~(HCPTR_CP(10) | HCPTR_CP(11))) | HCPTR_TTA | HCPTR_TAM,
      CPTR_EL2);

  /*
   * Configure HCR_EL2 with the bare minimum to run Xen until a guest
   * is scheduled. {A,I,F}MO bits are set to allow EL2 receiving
   * interrupts.
   */
  WRITE_SYSREG(HCR_AMO | HCR_FMO | HCR_IMO, HCR_EL2);
  isb();
}

void __div0(void) {
  panic("Division by zero in hypervisor.\n");
}

#ifndef IS_64BIT
static inline bool is_zero_register(int reg) {
  /* There is no zero register for ARM32 */
  return false;
}
#else
static inline bool is_zero_register(int reg) {
  /*
   * For store/load and sysreg instruction, the encoding 31 always
   * corresponds to {w,x}zr which is the zero register.
   */
  return (reg == 31);
}
#endif

/*
 * Returns a pointer to the given register value in regs, taking the
 * processor mode (CPSR) into account.
 *
 * Note that this function should not be used directly but via
 * {get,set}_user_reg.
 */
static u_register_t *select_user_reg(struct cpu_user_regs *regs, int reg) {
  DEBUG_ASSERT(!guest_mode(regs));
  thread_t *ct = get_current_thread();
  struct pcb_user_ctx *v = &ct->arch;

#ifndef IS_64BIT
  switch (reg) {
    case 0 ... 7: /* Unbanked registers */
      return &regs->r[reg];
    case 8 ... 12: /* Register banked in FIQ mode */
      if (fiq_mode(regs))
        return vcpu_read_reg_ptr(v, CTX_R8fiq + ARCH_BYTES_BITS * (reg - 8));
      else
        return &regs->r[reg];
    case 13 ... 14: /* Banked SP + LR registers */
      switch (regs->spsr & CPSR_MODE_MASK) {
        case CPSR_MODE_USR:
        case CPSR_MODE_SYS: /* Sys regs are the usr regs */
          if (reg == 13)
            return &regs->usp;
          else /* lr_usr == lr in a user frame */
            return &regs->ulr;
        case CPSR_MODE_FIQ:
          return vcpu_read_reg_ptr(v, CTX_SPfiq - ARCH_BYTES_BITS * (reg - 13));
        case CPSR_MODE_IRQ:
          return vcpu_read_reg_ptr(v, CTX_SPirq - ARCH_BYTES_BITS * (reg - 13));
        case CPSR_MODE_SVC:
          return vcpu_read_reg_ptr(v, CTX_SPsvc - ARCH_BYTES_BITS * (reg - 13));
        case CPSR_MODE_ABT:
          return vcpu_read_reg_ptr(v, CTX_SPabt - ARCH_BYTES_BITS * (reg - 13));
        case CPSR_MODE_UND:
          return vcpu_read_reg_ptr(v, CTX_SPund - ARCH_BYTES_BITS * (reg - 13));
        case CPSR_MODE_MON:
        case CPSR_MODE_HYP:
        default:
          panic("invalid reg mode\n");
      }
    case 15: /* PC */
      return &regs->pc;
    default:
      panic("invalid reg\n");
  }
#else
  /*
   * On 64-bit the syndrome register contains the register index as
   * viewed in AArch64 state even if the trap was from AArch32 mode.
   */
  DEBUG_ASSERT(is_zero_register(reg)); /* Cannot be {w,x}zr */
  return &regs->r[reg];
#endif
}

u_register_t get_user_reg(struct cpu_user_regs *regs, int reg) {
  if (is_zero_register(reg)) return 0;

  return *select_user_reg(regs, reg);
}

void set_user_reg(struct cpu_user_regs *regs, int reg, u_register_t value) {
  if (is_zero_register(reg)) return;

  *select_user_reg(regs, reg) = value;
}

static const char *decode_fsc(uint32_t fsc, int *level) {
  const char *msg = NULL;

  switch (fsc & 0x3f) {
    case FSC_FLT_TRANS ... FSC_FLT_TRANS + 3:
      msg = "Translation fault";
      *level = fsc & FSC_LL_MASK;
      break;
    case FSC_FLT_ACCESS ... FSC_FLT_ACCESS + 3:
      msg = "Access fault";
      *level = fsc & FSC_LL_MASK;
      break;
    case FSC_FLT_PERM ... FSC_FLT_PERM + 3:
      msg = "Permission fault";
      *level = fsc & FSC_LL_MASK;
      break;

    case FSC_SEA:
      msg = "Synchronous External Abort";
      break;
    case FSC_SPE:
      msg = "Memory Access Synchronous Parity Error";
      break;
    case FSC_APE:
      msg = "Memory Access Asynchronous Parity Error";
      break;
    case FSC_SEATT ... FSC_SEATT + 3:
      msg = "Sync. Ext. Abort Translation Table";
      *level = fsc & FSC_LL_MASK;
      break;
    case FSC_SPETT ... FSC_SPETT + 3:
      msg = "Sync. Parity. Error Translation Table";
      *level = fsc & FSC_LL_MASK;
      break;
    case FSC_AF:
      msg = "Alignment Fault";
      break;
    case FSC_DE:
      msg = "Debug Event";
      break;

    case FSC_LKD:
      msg = "Implementation Fault: Lockdown Abort";
      break;
    case FSC_CPR:
      msg = "Implementation Fault: Coprocossor Abort";
      break;

    default:
      msg = "Unknown Failure";
      break;
  }
  return msg;
}

static const char *fsc_level_str(int level) {
  switch (level) {
    case -1:
      return "";
    case 1:
      return " at level 1";
    case 2:
      return " at level 2";
    case 3:
      return " at level 3";
    default:
      return " (level invalid)";
  }
}

void panic_PAR(uint64_t par) {
  const char *msg;
  int level = -1;
  int stage = par & PAR_STAGE2 ? 2 : 1;
  int second_in_first = !!(par & PAR_STAGE21);

  msg = decode_fsc((par & PAR_FSC_MASK) >> PAR_FSC_SHIFT, &level);

  printf("PAR: %016llx: %s stage %d%s%s\n", par, msg, stage,
         second_in_first ? " during second stage lookup" : "",
         fsc_level_str(level));

  panic("Error during Hypervisor-to-physical address translation\n");
}

static void cpsr_switch_mode(struct cpu_user_regs *regs, int mode) {
  u_register_t sctlr = READ_SYSREG(SCTLR_EL1);

  regs->spsr &= ~(CPSR_MODE_MASK | CPSR_IT_MASK | CPSR_JAZELLE |
                  CPSR_BIG_ENDIAN | CPSR_THUMB);

  regs->spsr |= mode;
  regs->spsr |= CPSR_IRQ_MASK;
  if (mode == CPSR_MODE_ABT) regs->spsr |= CPSR_ABT_MASK;
  if (sctlr & SCTLR_TE_BIT) regs->spsr |= CPSR_THUMB;
  if (sctlr & SCTLR_EE_BIT) regs->spsr |= CPSR_BIG_ENDIAN;
}

static vaddr_t exception_handler32(vaddr_t offset) {
  u_register_t sctlr = READ_SYSREG(SCTLR_EL1);

  if (sctlr & SCTLR_V_BIT)
    return 0xffff0000 + offset;
  else /* always have security exceptions */
    return READ_SYSREG(VBAR_EL1) + offset;
}

/* Injects an Undefined Instruction exception into the current vcpu,
 * PC is the exact address of the faulting instruction (without
 * pipeline adjustments). See TakeUndefInstrException pseudocode in
 * ARM ARM.
 */
static void inject_undef32_exception(struct cpu_user_regs *regs) {
  uint32_t spsr = regs->spsr;
  int is_thumb = (regs->spsr & CPSR_THUMB);
  /* Saved PC points to the instruction past the faulting instruction. */
  uint32_t return_offset = is_thumb ? 2 : 4;
  thread_t *ct = get_current_thread();
  struct pcb_user_ctx *v = &ct->arch;

  /* Update processor mode */
  cpsr_switch_mode(regs, CPSR_MODE_UND);

  /* Update banked registers */
  vcpu_write_reg(v, CTX_SPSRund, spsr);
  vcpu_write_reg(v, CTX_LRund, regs->pc + return_offset);

  /* Branch to exception vector */
  regs->pc = exception_handler32(VECTOR32_UND);
}

/* Injects an Abort exception into the current vcpu, PC is the exact
 * address of the faulting instruction (without pipeline
 * adjustments). See TakePrefetchAbortException and
 * TakeDataAbortException pseudocode in ARM ARM.
 */
static void inject_abt32_exception(struct cpu_user_regs *regs, int prefetch,
                                   u_register_t addr) {
  uint32_t spsr = regs->spsr;
  int is_thumb = (regs->spsr & CPSR_THUMB);
  /* Saved PC points to the instruction past the faulting instruction. */
  uint32_t return_offset = is_thumb ? 4 : 0;
  u_register_t fsr;
  thread_t *ct = get_current_thread();
  struct pcb_user_ctx *v = &ct->arch;

  cpsr_switch_mode(regs, CPSR_MODE_ABT);

  /* Update banked registers */
  vcpu_write_reg(v, CTX_SPSRabt, spsr);
  vcpu_write_reg(v, CTX_LRabt, regs->pc + return_offset);

  regs->pc = exception_handler32(prefetch ? VECTOR32_PABT : VECTOR32_DABT);

  /* Inject a debug fault, best we can do right now */
  if (READ_SYSREG(TCR_EL1) & TTBCR_EAE)
    fsr = FSR_LPAE | FSRL_STATUS_DEBUG;
  else
    fsr = FSRS_FS_DEBUG;

  if (prefetch) {
    /* Set IFAR and IFSR */
#ifndef IS_64BIT
    WRITE_SYSREG(addr, IFAR);
    WRITE_SYSREG(fsr, IFSR);
#else
    /* FAR_EL1[63:32] is AArch32 register IFAR */
    u_register_t far = READ_SYSREG(FAR_EL1) & 0xffffffffUL;
    far |= addr << 32;
    WRITE_SYSREG(far, FAR_EL1);
    WRITE_SYSREG(fsr, IFSR32_EL2);
#endif
  } else {
#ifndef IS_64BIT
    /* Set DFAR and DFSR */
    WRITE_SYSREG(addr, DFAR);
    WRITE_SYSREG(fsr, DFSR);
#else
    /* FAR_EL1[31:0] is AArch32 register DFAR */
    u_register_t far = READ_SYSREG(FAR_EL1) & ~0xffffffffUL;
    far |= addr;
    WRITE_SYSREG(far, FAR_EL1);
    /* ESR_EL1 is AArch32 register DFSR */
    WRITE_SYSREG(fsr, ESR_EL1);
#endif
  }
}

static void inject_dabt32_exception(struct cpu_user_regs *regs,
                                    u_register_t addr) {
  inject_abt32_exception(regs, 0, addr);
}

static void inject_pabt32_exception(struct cpu_user_regs *regs,
                                    u_register_t addr) {
  inject_abt32_exception(regs, 1, addr);
}

#ifdef IS_64BIT
/*
 * Take care to call this while regs contains the original faulting
 * state and not the (partially constructed) exception state.
 */
static vaddr_t exception_handler64(struct cpu_user_regs *regs, vaddr_t offset) {
  vaddr_t base = READ_SYSREG(VBAR_EL1);

  if (usr_mode(regs))
    base += VECTOR64_LOWER32_BASE;
  else if (psr_mode(regs->spsr, CPSR_MODE_EL0t))
    base += VECTOR64_LOWER64_BASE;
  else /* Otherwise must be from kernel mode */
    base += VECTOR64_CURRENT_SPx_BASE;

  return base + offset;
}

/* Inject an undefined exception into a 64 bit guest */
void inject_undef64_exception(struct cpu_user_regs *regs, int instr_len) {
  vaddr_t handler;
  const union hsr esr = {
      .iss = 0,
      .len = instr_len,
      .ec = HSR_EC_UNKNOWN,
  };

  handler = exception_handler64(regs, VECTOR64_SYNC_OFFSET);

  regs->spsr_el1 = regs->spsr;
  regs->elr_el1 = regs->pc;

  regs->spsr = CPSR_MODE_EL1h | CPSR_ABT_MASK | CPSR_FIQ_MASK | CPSR_IRQ_MASK |
               CPSR_DBG_MASK;
  regs->pc = handler;

  WRITE_SYSREG(esr.bits, ESR_EL1);
}

/* Inject an abort exception into a 64 bit guest */
static void inject_abt64_exception(struct cpu_user_regs *regs, int prefetch,
                                   u_register_t addr, int instr_len) {
  vaddr_t handler;
  union hsr esr = {
      .iss = 0,
      .len = instr_len,
  };

  if (psr_mode_is_user(regs))
    esr.ec =
        prefetch ? HSR_EC_INSTR_ABORT_LOWER_EL : HSR_EC_DATA_ABORT_LOWER_EL;
  else
    esr.ec = prefetch ? HSR_EC_INSTR_ABORT_CURR_EL : HSR_EC_DATA_ABORT_CURR_EL;

  handler = exception_handler64(regs, VECTOR64_SYNC_OFFSET);

  regs->spsr_el1 = regs->spsr;
  regs->elr_el1 = regs->pc;

  regs->spsr = CPSR_MODE_EL1h | CPSR_ABT_MASK | CPSR_FIQ_MASK | CPSR_IRQ_MASK |
               CPSR_DBG_MASK;
  regs->pc = handler;

  WRITE_SYSREG(addr, FAR_EL1);
  WRITE_SYSREG(esr.bits, ESR_EL1);
}

static void inject_dabt64_exception(struct cpu_user_regs *regs,
                                    u_register_t addr, int instr_len) {
  inject_abt64_exception(regs, 0, addr, instr_len);
}

static void inject_iabt64_exception(struct cpu_user_regs *regs,
                                    u_register_t addr, int instr_len) {
  inject_abt64_exception(regs, 1, addr, instr_len);
}

#endif

void inject_undef_exception(struct cpu_user_regs *regs, const union hsr hsr) {
  if (is_32bit_domain) inject_undef32_exception(regs);
#ifdef IS_64BIT
  else
    inject_undef64_exception(regs, hsr.len);
#endif
}

static void inject_iabt_exception(struct cpu_user_regs *regs, u_register_t addr,
                                  int instr_len) {
  if (is_32bit_domain) inject_pabt32_exception(regs, addr);
#ifdef IS_64BIT
  else
    inject_iabt64_exception(regs, addr, instr_len);
#endif
}

static void inject_dabt_exception(struct cpu_user_regs *regs, u_register_t addr,
                                  int instr_len) {
  if (is_32bit_domain) inject_dabt32_exception(regs, addr);
#ifdef IS_64BIT
  else
    inject_dabt64_exception(regs, addr, instr_len);
#endif
}

/*
 * Inject a virtual Abort/SError into the guest.
 *
 * This should only be called with 'current'.
 */
static void inject_vabt_exception(struct cpu_user_regs *regs,
                                  struct pcb_user_ctx *v) {
  const union hsr hsr = {.bits = regs->hsr};

  /*
   * SVC/HVC/SMC already have an adjusted PC (See ARM ARM DDI 0487A.j
   * D1.10.1 for more details), which we need to correct in order to
   * return to after having injected the SError.
   */
  switch (hsr.ec) {
    case HSR_EC_SVC32:
    case HSR_EC_HVC32:
    case HSR_EC_SMC32:
#ifdef IS_64BIT
    case HSR_EC_SVC64:
    case HSR_EC_HVC64:
    case HSR_EC_SMC64:
#endif
      regs->pc -= hsr.len ? 4 : 2;
      break;

    default:
      break;
  }

  uint32_t hcr = read_hcr();
  hcr |= HCR_VA;
  write_hcr(hcr);
}

/*
 * SError exception handler.
 *
 * A true parameter "guest" means that the SError is type#1 or type#2.
 *
 * @guest indicates whether this is a SError generated by the guest.
 *
 * If true, the SError was generated by the guest, so it is safe to continue
 * and forward to the guest (if requested).
 *
 * If false, the SError was likely generated by the hypervisor. As we cannot
 * distinguish between precise and imprecise SErrors, it is not safe to
 * continue.
 *
 * Note that Arm32 asynchronous external abort generated by the
 * hypervisor will be handled in do_trap_data_abort().
 */
static void __do_trap_serror(struct cpu_user_regs *regs, bool guest) {
  thread_t *ct = get_current_thread();
  struct pcb_user_ctx *v = &ct->arch;

  /*
   * When using "DIVERSE", the SErrors generated by the guest will be
   * forwarded to the currently running vCPU.
   */
  if (guest)  // serrors_op == SERRORS_DIVERSE &&
    return inject_vabt_exception(regs, v);

  do_unexpected_trap("SError", regs);
}

void do_unexpected_trap(const char *msg, const struct cpu_user_regs *regs) {
  printf("CPU%d: Unexpected Trap: %s\n", arch_curr_cpu_num(), msg);
  // TODO: show_execution_state(regs);
  panic("CPU%d: Unexpected Trap: %s\n", arch_curr_cpu_num(), msg);
}

#ifdef IS_64BIT
static void do_trap_brk(struct cpu_user_regs *regs, const union hsr hsr) {
  do_unexpected_trap("Undefined Breakpoint Value", regs);
}
#endif

static u_register_t do_deprecated_hypercall(void) {
  // TODO: show hypercall number
  return -ENOSYS;
}

typedef u_register_t (*arm_hypercall_fn_t)(u_register_t, u_register_t,
                                           u_register_t, u_register_t,
                                           u_register_t);

typedef struct {
  arm_hypercall_fn_t fn;
  int nr_args;
} arm_hypercall_t;

#define HYPERCALL(_name, _nr_args)             \
  [__HYPERVISOR_##_name] = {                   \
      .fn = (arm_hypercall_fn_t) & do_##_name, \
      .nr_args = _nr_args,                     \
  }

#define HYPERCALL_ARM(_name, _nr_args)             \
  [__HYPERVISOR_##_name] = {                       \
      .fn = (arm_hypercall_fn_t) & do_arm_##_name, \
      .nr_args = _nr_args,                         \
  }
/*
 * Only use this for hypercalls which were deprecated (i.e. replaced
 * by something else) before Xen on ARM was created, i.e. *not* for
 * hypercalls which are simply not yet used on ARM.
 */
#define HYPERCALL_DEPRECATED(_name, _nr_args)               \
  [__HYPERVISOR_##_name] = {                                \
      .fn = (arm_hypercall_fn_t) & do_deprecated_hypercall, \
      .nr_args = _nr_args,                                  \
  }

static arm_hypercall_t arm_hypercall_table[] = {};

#ifndef NDEBUG
static void do_debug_trap(struct cpu_user_regs *regs, unsigned int code) {
  unsigned int reg;
  switch (code) {
    case 0xe0 ... 0xef:
      reg = code - 0xe0;
      printf("R%d = 0x%x at 0x%x\n", reg,
             get_user_reg(regs, reg), regs->pc);
      break;
    case 0xfd:
      printf("Reached %x\n", regs->pc);
      break;
    case 0xfe:
      printf("%c", (char)(get_user_reg(regs, 0) & 0xff));
      break;
    case 0xff:
      // TODO: show_execution_state(regs);
      break;
    default:
      printf("Unhandled debug trap %#x\n", code);
      break;
  }
}
#endif

#define HYPERCALL_RESULT_REG(regs) (regs)->r[0]
#define HYPERCALL_ARG1(regs) (regs)->r[0]
#define HYPERCALL_ARG2(regs) (regs)->r[1]
#define HYPERCALL_ARG3(regs) (regs)->r[2]
#define HYPERCALL_ARG4(regs) (regs)->r[3]
#define HYPERCALL_ARG5(regs) (regs)->r[4]
#define HYPERCALL_ARGS(regs) (regs)->r[0], (regs)->r[1], (regs)->r[2], (regs)->r[3], (regs)->r[4]

static void do_trap_hypercall(struct cpu_user_regs *regs, u_register_t *nr,
                              const union hsr hsr) {
  arm_hypercall_fn_t call = NULL;

  if (*nr >= ARRAY_SIZE(arm_hypercall_table)) {
    HYPERCALL_RESULT_REG(regs) = -ENOSYS;
    return;
  }

  // curr->hcall_preempted = false;

  call = arm_hypercall_table[*nr].fn;
  if (call == NULL) {
    HYPERCALL_RESULT_REG(regs) = -ENOSYS;
    return;
  }

  HYPERCALL_RESULT_REG(regs) = call(HYPERCALL_ARGS(regs));

#if 0
    if (!curr->hcall_preempted)
    {
        /* Deliberately corrupt parameter regs used by this hypercall. */
        switch (arm_hypercall_table[*nr].nr_args)
        {
        case 5:
            HYPERCALL_ARG5(regs) = 0xDEADBEEF;
        case 4:
            HYPERCALL_ARG4(regs) = 0xDEADBEEF;
        case 3:
            HYPERCALL_ARG3(regs) = 0xDEADBEEF;
        case 2:
            HYPERCALL_ARG2(regs) = 0xDEADBEEF;
        case 1: /* Don't clobber x0/r0 -- it's the return value */
            break;
        default:
            panic();
        }
        *nr = 0xDEADBEEF;
    }
#endif

  /* Ensure the hypercall trap instruction is re-executed. */
  // if (curr->hcall_preempted)
  //     regs->pc -= 4; /* re-execute 'hvc #XEN_HYPERCALL_TAG' */
}

/*
 * stolen from arch/arm/kernel/opcodes.c
 *
 * condition code lookup table
 * index into the table is test code: EQ, NE, ... LT, GT, AL, NV
 *
 * bit position in short is condition code: NZCV
 */
static const unsigned short cc_map[16] = {
    0xF0F0, /* EQ == Z set            */
    0x0F0F, /* NE                     */
    0xCCCC, /* CS == C set            */
    0x3333, /* CC                     */
    0xFF00, /* MI == N set            */
    0x00FF, /* PL                     */
    0xAAAA, /* VS == V set            */
    0x5555, /* VC                     */
    0x0C0C, /* HI == C set && Z clear */
    0xF3F3, /* LS == C clear || Z set */
    0xAA55, /* GE == (N==V)           */
    0x55AA, /* LT == (N!=V)           */
    0x0A05, /* GT == (!Z && (N==V))   */
    0xF5FA, /* LE == (Z || (N!=V))    */
    0xFFFF, /* AL always              */
    0       /* NV                     */
};

int check_conditional_instr(struct cpu_user_regs *regs, const union hsr hsr) {
  u_register_t spsr, cpsr_cond;
  int cond;

  /*
   * SMC32 instruction case is special. Under SMC32 we mean SMC
   * instruction on ARMv7 or SMC instruction originating from
   * AArch32 state on ARMv8.
   * On ARMv7 it will be trapped only if it passed condition check
   * (ARM DDI 0406C.c page B3-1431), but we need to check condition
   * flags on ARMv8 (ARM DDI 0487B.a page D7-2271).
   * Encoding for HSR.ISS on ARMv8 is backwards compatible with ARMv7:
   * HSR.ISS is defined as UNK/SBZP on ARMv7 which means, that it
   * will be read as 0. This includes CCKNOWNPASS field.
   * If CCKNOWNPASS == 0 then this was an unconditional instruction or
   * it has passed conditional check (ARM DDI 0487B.a page D7-2272).
   */
  if (hsr.ec == HSR_EC_SMC32 && hsr.smc32.ccknownpass == 0) return 1;

  /* Unconditional Exception classes */
  if (hsr.ec == HSR_EC_UNKNOWN || (hsr.ec >= 0x10 && hsr.ec != HSR_EC_SMC32))
    return 1;

  /* Check for valid condition in hsr */
  cond = hsr.cond.ccvalid ? hsr.cond.cc : -1;

  /* Unconditional instruction */
  if (cond == 0xe) return 1;

  spsr = regs->spsr;

  /* If cc is not valid then we need to examine the IT state */
  if (cond < 0) {
    unsigned long it;

    DEBUG_ASSERT(!psr_mode_is_32bit(regs) || !(spsr & CPSR_THUMB));

    it = ((spsr >> (10 - 2)) & 0xfc) | ((spsr >> 25) & 0x3);

    /* it == 0 => unconditional. */
    if (it == 0) return 1;

    /* The cond for this instruction works out as the top 4 bits. */
    cond = (it >> 4);
  }

  cpsr_cond = spsr >> 28;

  if (!((cc_map[cond] >> cpsr_cond) & 1)) {
    return 0;
  }
  return 1;
}

void advance_pc(struct cpu_user_regs *regs, const union hsr hsr) {
  u_register_t itbits, cond, spsr = regs->spsr;
  bool is_thumb = psr_mode_is_32bit(regs) && (spsr & CPSR_THUMB);

  if (is_thumb && (spsr & CPSR_IT_MASK)) {
    /* The ITSTATE[7:0] block is contained in CPSR[15:10],CPSR[26:25]
     *
     * ITSTATE[7:5] are the condition code
     * ITSTATE[4:0] are the IT bits
     *
     * If the condition is non-zero then the IT state machine is
     * advanced by shifting the IT bits left.
     *
     * See A2-51 and B1-1148 of DDI 0406C.b.
     */
    cond = (spsr & 0xe000) >> 13;
    itbits = (spsr & 0x1c00) >> (10 - 2);
    itbits |= (spsr & (0x3 << 25)) >> 25;

    if ((itbits & 0x7) == 0)
      itbits = cond = 0;
    else
      itbits = (itbits << 1) & 0x1f;

    spsr &= ~CPSR_IT_MASK;
    spsr |= cond << 13;
    spsr |= (itbits & 0x1c) << (10 - 2);
    spsr |= (itbits & 0x3) << 25;

    regs->spsr = spsr;
  }

  regs->pc += hsr.len ? 4 : 2;
}

/* Read as zero and write ignore */
void handle_raz_wi(struct cpu_user_regs *regs, int regidx, bool read,
                   const union hsr hsr, int min_el) {
  ASSERT((min_el == 0) || (min_el == 1));

  if (min_el > 0 && psr_mode_is_user(regs))
    return inject_undef_exception(regs, hsr);

  if (read) set_user_reg(regs, regidx, 0);
  /* else: write ignored */

  advance_pc(regs, hsr);
}

/* write only as write ignore */
void handle_wo_wi(struct cpu_user_regs *regs, int regidx, bool read,
                  const union hsr hsr, int min_el) {
  ASSERT((min_el == 0) || (min_el == 1));

  if (min_el > 0 && psr_mode_is_user(regs))
    return inject_undef_exception(regs, hsr);

  if (read) return inject_undef_exception(regs, hsr);
  /* else: ignore */

  advance_pc(regs, hsr);
}

/* Read only as value provided with 'val' argument of this function */
void handle_ro_read_val(struct cpu_user_regs *regs, int regidx, bool read,
                        const union hsr hsr, int min_el, u_register_t val) {
  ASSERT((min_el == 0) || (min_el == 1));

  if (min_el > 0 && psr_mode_is_user(regs))
    return inject_undef_exception(regs, hsr);

  if (!read) return inject_undef_exception(regs, hsr);

  set_user_reg(regs, regidx, val);

  advance_pc(regs, hsr);
}

/* Read only as read as zero */
inline void handle_ro_raz(struct cpu_user_regs *regs, int regidx, bool read,
                          const union hsr hsr, int min_el) {
  handle_ro_read_val(regs, regidx, read, hsr, min_el, 0);
}

/*
 * Return the value of the hypervisor fault address register.
 *
 * On ARM32, the register will be different depending whether the
 * fault is a prefetch abort or data abort.
 */
static inline vaddr_t get_hfar(bool is_data) {
  vaddr_t gva;

#ifndef IS_64BIT
  if (is_data)
    gva = READ_CP32(HDFAR);
  else
    gva = READ_CP32(HIFAR);
#else
  gva = READ_SYSREG(FAR_EL2);
#endif

  return gva;
}

static inline paddr_t get_faulting_ipa(vaddr_t gva) {
  u_register_t hpfar = READ_SYSREG(HPFAR_EL2);
  paddr_t ipa;

  ipa = (paddr_t)(hpfar & HPFAR_MASK) << (12 - 4);
  ipa |= gva & ~(PAGE_SIZE-1);

  return ipa;
}

static inline bool hpfar_is_valid(bool s1ptw, uint8_t fsc) {
  /*
   * HPFAR is valid if one of the following cases are true:
   *  1. the stage 2 fault happen during a stage 1 page table walk
   *  (the bit ESR_EL2.S1PTW is set)
   *  2. the fault was due to a translation fault and the processor
   *  does not carry erratum #8342220
   *
   * Note that technically HPFAR is valid for other cases, but they
   * are currently not supported by Xen.
   */
  return s1ptw || (fsc == FSC_FLT_TRANS);  // && !check_workaround_834220()
}

static void do_trap_stage2_abort_guest(struct cpu_user_regs *regs,
                                       const union hsr hsr) {
  /*
   * The encoding of hsr_iabt is a subset of hsr_dabt. So use
   * hsr_dabt to represent an abort fault.
   */
  const struct hsr_xabt xabt = hsr.xabt;
  vaddr_t gva;
  paddr_t gpa;
  uint8_t fsc = xabt.fsc & ~FSC_LL_MASK;
  bool is_data = (hsr.ec == HSR_EC_DATA_ABORT_LOWER_EL);

  /*
   * If this bit has been set, it means that this stage-2 abort is caused
   * by a guest external abort. We treat this stage-2 abort as guest SError.
   */
  if (xabt.eat) return __do_trap_serror(regs, true);

  gva = get_hfar(is_data);

  if (hpfar_is_valid(xabt.s1ptw, fsc))
    gpa = get_faulting_ipa(gva);
  else {
    /*
     * Flush the TLB to make sure the DTLB is clear before
     * doing GVA->IPA translation. If we got here because of
     * an entry only present in the ITLB, this translation may
     * still be inaccurate.
     */
    return;
  }

  switch (fsc) {
    case FSC_FLT_PERM: {
      /*
       * The only way to get here right now is because of mem_access,
       * thus reinjecting the exception to the guest is never required.
       */
      return;
    }
    case FSC_FLT_TRANS:
      /*
       * Attempt first to emulate the MMIO as the data abort will
       * likely happen in an emulated region.
       *
       * Note that emulated region cannot be executed
       */
      break;
    default:
      dprintf(ALWAYS, "Unsupported FSC: HSR=%lx DFSC=%x\n",
              hsr.bits, xabt.fsc);
  }

inject_abt:
  dprintf(ALWAYS,
          "HSR=%lx pc=%x gva=%lx gpa=%lx\n",
          hsr.bits, regs->pc, gva, gpa);
  if (is_data)
    inject_dabt_exception(regs, gva, hsr.len);
  else
    inject_iabt_exception(regs, gva, hsr.len);
}

void do_trap_guest_sync(struct cpu_user_regs *regs) {
  const union hsr hsr = {.bits = regs->hsr};

  switch (hsr.ec) {
    case HSR_EC_WFI_WFE:
      /*
       * HCR_EL2.TWI, HCR_EL2.TWE
       *
       * ARMv7 (DDI 0406C.b): B1.14.9
       * ARMv8 (DDI 0487A.d): D1-1505 Table D1-51
       */
      if (!check_conditional_instr(regs, hsr)) {
        advance_pc(regs, hsr);
        return;
      }
      if (hsr.wfi_wfe.ti) {
        /* Yield the VCPU for WFE */
        thread_yield();
      } else {
        /* Block the VCPU for WFI */
        thread_sleep(1);
      }
      advance_pc(regs, hsr);
      break;
    case HSR_EC_CP15_32:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp15_32(regs, hsr);
      break;
    case HSR_EC_CP15_64:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp15_64(regs, hsr);
      break;
    case HSR_EC_CP14_32:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp14_32(regs, hsr);
      break;
    case HSR_EC_CP14_64:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp14_64(regs, hsr);
      break;
    case HSR_EC_CP14_DBG:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp14_dbg(regs, hsr);
      break;
    case HSR_EC_CP10:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp10(regs, hsr);
      break;
    case HSR_EC_CP:
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_cp(regs, hsr);
      break;
    case HSR_EC_SMC32:
      /*
       * HCR_EL2.TSC
       *
       * ARMv7 (DDI 0406C.b): B1.14.8
       * ARMv8 (DDI 0487A.d): D1-1501 Table D1-44
       */
      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
      do_trap_smc(regs, hsr);
      break;
    case HSR_EC_HVC32: {
      u_register_t nr;

      DEBUG_ASSERT(!psr_mode_is_32bit(regs));
#ifndef NDEBUG
      if ((hsr.iss & 0xff00) == 0xff00)
        return do_debug_trap(regs, hsr.iss & 0x00ff);
#endif
      if (hsr.iss == 0) return do_trap_hvc_smccc(regs);
      nr = regs->r[12];
      do_trap_hypercall(regs, &nr, hsr);
      regs->r[12] = (uint32_t)nr;
      break;
    }
#ifdef IS_64BIT
    case HSR_EC_HVC64:
      DEBUG_ASSERT(psr_mode_is_32bit(regs));
#ifndef NDEBUG
      if ((hsr.iss & 0xff00) == 0xff00)
        return do_debug_trap(regs, hsr.iss & 0x00ff);
#endif
      if (hsr.iss == 0) return do_trap_hvc_smccc(regs);
      do_trap_hypercall(regs, &regs->x16, hsr);
      break;
    case HSR_EC_SMC64:
      /*
       * HCR_EL2.TSC
       *
       * ARMv8 (DDI 0487A.d): D1-1501 Table D1-44
       */
      DEBUG_ASSERT(psr_mode_is_32bit(regs));
      do_trap_smc(regs, hsr);
      break;
    case HSR_EC_SYSREG:
      DEBUG_ASSERT(psr_mode_is_32bit(regs));
      do_sysreg(regs, hsr);
      break;
#endif

    case HSR_EC_INSTR_ABORT_LOWER_EL:
      do_trap_stage2_abort_guest(regs, hsr);
      break;
    case HSR_EC_DATA_ABORT_LOWER_EL:
      do_trap_stage2_abort_guest(regs, hsr);
      break;

    default:
      dprintf(ALWAYS,
              "Unknown Guest Trap. HSR=%lx EC=0x%x IL=%x Syndrome=0x%x\n",
              hsr.bits, hsr.ec, hsr.len, hsr.iss);
      inject_undef_exception(regs, hsr);
  }
}

void do_trap_hyp_serror(struct cpu_user_regs *regs) {
  __do_trap_serror(regs, false);
}

void do_trap_guest_serror(struct cpu_user_regs *regs) {
  __do_trap_serror(regs, true);
}
