#ifndef __ASM_ARM_VTRAPS__
#define __ASM_ARM_VTRAPS__

#ifdef IS_64BIT
#include <arch/arm64.h>
#define cpu_user_regs arm64_iframe_long
#else
#include <arch/arm.h>
#define cpu_user_regs arm_fault_frame
#endif
#include <asm/hsr.h>
#include <kern/bitops.h>


int check_conditional_instr(struct cpu_user_regs *regs, const union hsr hsr);

void advance_pc(struct cpu_user_regs *regs, const union hsr hsr);

void inject_undef_exception(struct cpu_user_regs *regs, const union hsr hsr);

/* read as zero and write ignore */
void handle_raz_wi(struct cpu_user_regs *regs, int regidx, bool read,
                   const union hsr hsr, int min_el);

/* write only as write ignore */
void handle_wo_wi(struct cpu_user_regs *regs, int regidx, bool read,
                  const union hsr hsr, int min_el);

/* read only as read as zero */
void handle_ro_raz(struct cpu_user_regs *regs, int regidx, bool read,
                   const union hsr hsr, int min_el);

/* Read only as value provided with 'val' argument */
void handle_ro_read_val(struct cpu_user_regs *regs, int regidx, bool read,
                        const union hsr hsr, int min_el, u_register_t val);

/* Co-processor registers emulation (see arch/arm/vcpreg.c). */
void do_cp15_32(struct cpu_user_regs *regs, const union hsr hsr);
void do_cp15_64(struct cpu_user_regs *regs, const union hsr hsr);
void do_cp14_32(struct cpu_user_regs *regs, const union hsr hsr);
void do_cp14_64(struct cpu_user_regs *regs, const union hsr hsr);
void do_cp14_dbg(struct cpu_user_regs *regs, const union hsr hsr);
void do_cp10(struct cpu_user_regs *regs, const union hsr hsr);
void do_cp(struct cpu_user_regs *regs, const union hsr hsr);

/* SMCCC handling */
void do_trap_smc(struct cpu_user_regs *regs, const union hsr hsr);
void do_trap_hvc_smccc(struct cpu_user_regs *regs);

int do_bug_frame(const struct cpu_user_regs *regs, vaddr_t pc);

void do_unexpected_trap(const char *msg, const struct cpu_user_regs *regs);

void set_user_reg(struct cpu_user_regs *regs, int reg, u_register_t value);
u_register_t get_user_reg(struct cpu_user_regs *regs, int reg);

#endif
