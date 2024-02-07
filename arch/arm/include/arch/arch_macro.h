/*
 * Copyright (c) 2016-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <kern/utils.h>

/*
 * AArch32 Co-processor registers.
 *
 * Note that AArch64 requires many of these definitions in order to
 * support 32-bit guests.
 */

#define __HSR_CPREG_c0  0
#define __HSR_CPREG_c1  1
#define __HSR_CPREG_c2  2
#define __HSR_CPREG_c3  3
#define __HSR_CPREG_c4  4
#define __HSR_CPREG_c5  5
#define __HSR_CPREG_c6  6
#define __HSR_CPREG_c7  7
#define __HSR_CPREG_c8  8
#define __HSR_CPREG_c9  9
#define __HSR_CPREG_c10 10
#define __HSR_CPREG_c11 11
#define __HSR_CPREG_c12 12
#define __HSR_CPREG_c13 13
#define __HSR_CPREG_c14 14
#define __HSR_CPREG_c15 15

#define __HSR_CPREG_0   0
#define __HSR_CPREG_1   1
#define __HSR_CPREG_2   2
#define __HSR_CPREG_3   3
#define __HSR_CPREG_4   4
#define __HSR_CPREG_5   5
#define __HSR_CPREG_6   6
#define __HSR_CPREG_7   7

#define _HSR_CPREG32(cp,op1,crn,crm,op2) \
    ((__HSR_CPREG_##crn) << HSR_CP32_CRN_SHIFT) | \
    ((__HSR_CPREG_##crm) << HSR_CP32_CRM_SHIFT) | \
    ((__HSR_CPREG_##op1) << HSR_CP32_OP1_SHIFT) | \
    ((__HSR_CPREG_##op2) << HSR_CP32_OP2_SHIFT)

#define _HSR_CPREG64(cp,op1,crm) \
    ((__HSR_CPREG_##crm) << HSR_CP64_CRM_SHIFT) | \
    ((__HSR_CPREG_##op1) << HSR_CP64_OP1_SHIFT)

/* Encode a register as per HSR ISS pattern */
#define HSR_CPREG32(X...) _HSR_CPREG32(X)
#define HSR_CPREG64(X...) _HSR_CPREG64(X)

#define HSR_EC_SHIFT                26

#define HSR_EC_UNKNOWN              0x00
#define HSR_EC_WFI_WFE              0x01
#define HSR_EC_CP15_32              0x03
#define HSR_EC_CP15_64              0x04
#define HSR_EC_CP14_32              0x05        /* Trapped MCR or MRC access to CP14 */
#define HSR_EC_CP14_DBG             0x06        /* Trapped LDC/STC access to CP14 (only for debug registers) */
#define HSR_EC_CP                   0x07        /* HCPTR-trapped access to CP0-CP13 */
#define HSR_EC_CP10                 0x08
#define HSR_EC_JAZELLE              0x09
#define HSR_EC_BXJ                  0x0a
#define HSR_EC_CP14_64              0x0c
#define HSR_EC_SVC32                0x11
#define HSR_EC_HVC32                0x12
#define HSR_EC_SMC32                0x13
#ifdef CONFIG_ARM_64
#define HSR_EC_SVC64                0x15
#define HSR_EC_HVC64                0x16
#define HSR_EC_SMC64                0x17
#define HSR_EC_SYSREG               0x18
#endif
#define HSR_EC_INSTR_ABORT_LOWER_EL 0x20
#define HSR_EC_INSTR_ABORT_CURR_EL  0x21
#define HSR_EC_DATA_ABORT_LOWER_EL  0x24
#define HSR_EC_DATA_ABORT_CURR_EL   0x25
#ifdef CONFIG_ARM_64
#define HSR_EC_BRK                  0x3c
#endif

/* Exception Vector offsets */
/* ... ARM32 */
#define VECTOR32_RST  0
#define VECTOR32_UND  4
#define VECTOR32_SVC  8
#define VECTOR32_PABT 12
#define VECTOR32_DABT 16
/* ... ARM64 */
#define VECTOR64_CURRENT_SP0_BASE  0x000
#define VECTOR64_CURRENT_SPx_BASE  0x200
#define VECTOR64_LOWER64_BASE      0x400
#define VECTOR64_LOWER32_BASE      0x600

#define VECTOR64_SYNC_OFFSET       0x000
#define VECTOR64_IRQ_OFFSET        0x080
#define VECTOR64_FIQ_OFFSET        0x100
#define VECTOR64_ERROR_OFFSET      0x180


/* Fault Status Register */
/*
 * 543210 BIT
 * 00XXLL -- XX Fault Level LL
 * ..01LL -- Translation Fault LL
 * ..10LL -- Access Fault LL
 * ..11LL -- Permission Fault LL
 * 01xxxx -- Abort/Parity
 * 10xxxx -- Other
 * 11xxxx -- Implementation Defined
 */
#define FSC_TYPE_MASK (U(0x3)<<4)
#define FSC_TYPE_FAULT (U(0x00)<<4)
#define FSC_TYPE_ABT   (U(0x01)<<4)
#define FSC_TYPE_OTH   (U(0x02)<<4)
#define FSC_TYPE_IMPL  (U(0x03)<<4)

#define FSC_FLT_TRANS  (0x04)
#define FSC_FLT_ACCESS (0x08)
#define FSC_FLT_PERM   (0x0c)
#define FSC_SEA        (0x10) /* Synchronous External Abort */
#define FSC_SPE        (0x18) /* Memory Access Synchronous Parity Error */
#define FSC_APE        (0x11) /* Memory Access Asynchronous Parity Error */
#define FSC_SEATT      (0x14) /* Sync. Ext. Abort Translation Table */
#define FSC_SPETT      (0x1c) /* Sync. Parity. Error Translation Table */
#define FSC_AF         (0x21) /* Alignment Fault */
#define FSC_DE         (0x22) /* Debug Event */
#define FSC_LKD        (0x34) /* Lockdown Abort */
#define FSC_CPR        (0x3a) /* Coprocossor Abort */

#define FSC_LL_MASK    (U(0x03)<<0)

/* FSR format, common */
#define FSR_LPAE                (UL(1)<<9)
/* FSR short format */
#define FSRS_FS_DEBUG           (UL(0)<<10|UL(0x2)<<0)
/* FSR long format */
#define FSRL_STATUS_DEBUG       (UL(0x22)<<0)

/* Physical Address Register */
#define PAR_F           (U(1)<<0)

/* .... If F == 1 */
#define PAR_FSC_SHIFT   (1)
#define PAR_FSC_MASK    (U(0x3f)<<PAR_FSC_SHIFT)
#define PAR_STAGE21     (U(1)<<8)     /* Stage 2 Fault During Stage 1 Walk */
#define PAR_STAGE2      (U(1)<<9)     /* Stage 2 Fault */

/* If F == 0 */
#define PAR_MAIR_SHIFT  56                       /* Memory Attributes */
#define PAR_MAIR_MASK   (0xffLL<<PAR_MAIR_SHIFT)
#define PAR_NS          (U(1)<<9)                   /* Non-Secure */
#define PAR_SH_SHIFT    7                        /* Shareability */
#define PAR_SH_MASK     (U(3)<<PAR_SH_SHIFT)

#define TTBCR_N      (7U << 0) /* TTBCR.EAE==0 */
#define TTBCR_T0SZ   (7U << 0) /* TTBCR.EAE==1 */
#define TTBCR_PD0    (1U << 4)
#define TTBCR_PD1    (1U << 5)
#define TTBCR_EPD0   (1U << 7)
#define TTBCR_IRGN0  (3U << 8)
#define TTBCR_ORGN0  (3U << 10)
#define TTBCR_SH0    (3U << 12)
#define TTBCR_T1SZ   (3U << 16)
#define TTBCR_A1     (1U << 22)
#define TTBCR_EPD1   (1U << 23)
#define TTBCR_IRGN1  (3U << 24)
#define TTBCR_ORGN1  (3U << 26)
#define TTBCR_SH1    (1U << 28)
#define TTBCR_EAE    (1U << 31)

/* HPFAR_EL2: Hypervisor IPA Fault Address Register */
#ifdef IS_64BIT
#define HPFAR_MASK	GENMASK(39, 4)
#else
#define HPFAR_MASK	GENMASK(31, 4)
#endif

/*******************************************************************************
 * MIDR bit definitions
 ******************************************************************************/
#define MIDR_IMPL_MASK		U(0xff)
#define MIDR_IMPL_SHIFT		U(24)
#define MIDR_VAR_SHIFT		U(20)
#define MIDR_VAR_BITS		U(4)
#define MIDR_REV_SHIFT		U(0)
#define MIDR_REV_BITS		U(4)
#define MIDR_PN_MASK		U(0xfff)
#define MIDR_PN_SHIFT		U(4)

/*******************************************************************************
 * MPIDR macros
 ******************************************************************************/
#define MPIDR_MT_MASK		(U(1) << 24)
#define MPIDR_CPU_MASK		MPIDR_AFFLVL_MASK
#define MPIDR_CLUSTER_MASK	(MPIDR_AFFLVL_MASK << MPIDR_AFFINITY_BITS)
#define MPIDR_AFFINITY_BITS	U(8)
#define MPIDR_AFFLVL_MASK	U(0xff)
#define MPIDR_AFFLVL_SHIFT	U(3)
#define MPIDR_AFF0_SHIFT	U(0)
#define MPIDR_AFF1_SHIFT	U(8)
#define MPIDR_AFF2_SHIFT	U(16)
#define MPIDR_AFF_SHIFT(_n)	MPIDR_AFF##_n##_SHIFT
#define MPIDR_AFFINITY_MASK	U(0x00ffffff)
#define MPIDR_AFFLVL0		U(0)
#define MPIDR_AFFLVL1		U(1)
#define MPIDR_AFFLVL2		U(2)
#define MPIDR_AFFLVL(_n)	MPIDR_AFFLVL##_n

#define MPIDR_AFFLVL0_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL1_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL2_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL3_VAL(mpidr)	U(0)

#define MPIDR_AFF_ID(mpid, n)					\
	(((mpid) >> MPIDR_AFF_SHIFT(n)) & MPIDR_AFFLVL_MASK)

#define MPID_MASK		(MPIDR_MT_MASK				|\
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT)|\
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT)|\
				 (MPIDR_AFFLVL_MASK << MPIDR_AFF0_SHIFT))

/*
 * An invalid MPID. This value can be used by functions that return an MPID to
 * indicate an error.
 */
#define INVALID_MPID		U(0xFFFFFFFF)

/*
 * The MPIDR_MAX_AFFLVL count starts from 0. Take care to
 * add one while using this macro to define array sizes.
 */
#define MPIDR_MAX_AFFLVL	U(2)

/* Data Cache set/way op type defines */
#define DC_OP_ISW			U(0x0)
#define DC_OP_CISW			U(0x1)
#if ERRATA_A53_827319
#define DC_OP_CSW			DC_OP_CISW
#else
#define DC_OP_CSW			U(0x2)
#endif

/*******************************************************************************
 * Generic timer memory mapped registers & offsets
 ******************************************************************************/
#define CNTCR_OFF			U(0x000)
/* Counter Count Value Lower register */
#define CNTCVL_OFF			U(0x008)
/* Counter Count Value Upper register */
#define CNTCVU_OFF			U(0x00C)
#define CNTFID_OFF			U(0x020)

#define CNTCR_EN			(U(1) << 0)
#define CNTCR_HDBG			(U(1) << 1)
#define CNTCR_FCREQ(x)			((x) << 8)

/*******************************************************************************
 * System register bit definitions
 ******************************************************************************/
/* CLIDR definitions */
#define LOUIS_SHIFT		U(21)
#define LOC_SHIFT		U(24)
#define CLIDR_FIELD_WIDTH	U(3)

/* CSSELR definitions */
#define LEVEL_SHIFT		U(1)

/* ID_DFR0_EL1 definitions */
#define ID_DFR0_COPTRC_SHIFT		U(12)
#define ID_DFR0_COPTRC_MASK		U(0xf)
#define ID_DFR0_COPTRC_SUPPORTED	U(1)
#define ID_DFR0_COPTRC_LENGTH		U(4)
#define ID_DFR0_TRACEFILT_SHIFT		U(28)
#define ID_DFR0_TRACEFILT_MASK		U(0xf)
#define ID_DFR0_TRACEFILT_SUPPORTED	U(1)
#define ID_DFR0_TRACEFILT_LENGTH	U(4)

/* ID_DFR1_EL1 definitions */
#define ID_DFR1_MTPMU_SHIFT	U(0)
#define ID_DFR1_MTPMU_MASK	U(0xf)
#define ID_DFR1_MTPMU_SUPPORTED	U(1)

/* ID_MMFR4 definitions */
#define ID_MMFR4_CNP_SHIFT	U(12)
#define ID_MMFR4_CNP_LENGTH	U(4)
#define ID_MMFR4_CNP_MASK	U(0xf)

#define ID_MMFR4_CCIDX_SHIFT	U(24)
#define ID_MMFR4_CCIDX_LENGTH	U(4)
#define ID_MMFR4_CCIDX_MASK	U(0xf)

/* ID_PFR0 definitions */
#define ID_PFR0_AMU_SHIFT	U(20)
#define ID_PFR0_AMU_LENGTH	U(4)
#define ID_PFR0_AMU_MASK	U(0xf)
#define ID_PFR0_AMU_NOT_SUPPORTED	U(0x0)
#define ID_PFR0_AMU_V1		U(0x1)
#define ID_PFR0_AMU_V1P1	U(0x2)

#define ID_PFR0_DIT_SHIFT	U(24)
#define ID_PFR0_DIT_LENGTH	U(4)
#define ID_PFR0_DIT_MASK	U(0xf)
#define ID_PFR0_DIT_SUPPORTED	(U(1) << ID_PFR0_DIT_SHIFT)

/* ID_PFR1 definitions */
#define ID_PFR1_VIRTEXT_SHIFT	U(12)
#define ID_PFR1_VIRTEXT_MASK	U(0xf)
#define GET_VIRT_EXT(id)	(((id) >> ID_PFR1_VIRTEXT_SHIFT) \
				 & ID_PFR1_VIRTEXT_MASK)
#define ID_PFR1_GENTIMER_SHIFT	U(16)
#define ID_PFR1_GENTIMER_MASK	U(0xf)
#define ID_PFR1_GIC_SHIFT	U(28)
#define ID_PFR1_GIC_MASK	U(0xf)
#define ID_PFR1_SEC_SHIFT	U(4)
#define ID_PFR1_SEC_MASK	U(0xf)
#define ID_PFR1_ELx_ENABLED	U(1)

/* SCTLR definitions */
#define SCTLR_RES1_DEF		((U(1) << 23) | (U(1) << 22) | (U(1) << 4) | \
				 (U(1) << 3))
#if ARM_ARCH_MAJOR == 7
#define SCTLR_RES1		SCTLR_RES1_DEF
#else
#define SCTLR_RES1		(SCTLR_RES1_DEF | (U(1) << 11))
#endif
#define SCTLR_M_BIT		(U(1) << 0) // MMU/MPU
#define SCTLR_A_BIT		(U(1) << 1)
#define SCTLR_C_BIT		(U(1) << 2)
#define SCTLR_CP15BEN_BIT	(U(1) << 5)
#define SCTLR_ITD_BIT		(U(1) << 7)
#define SCTLR_Z_BIT		(U(1) << 11)
#define SCTLR_I_BIT		(U(1) << 12)
#define SCTLR_V_BIT		(U(1) << 13)
#define SCTLR_RR_BIT		(U(1) << 14)
#define SCTLR_NTWI_BIT		(U(1) << 16)
#define SCTLR_BR_BIT		(U(1) << 17)
#define SCTLR_NTWE_BIT		(U(1) << 18)
#define SCTLR_WXN_BIT		(U(1) << 19)
#define SCTLR_UWXN_BIT		(U(1) << 20)
#define SCTLR_EE_BIT		(U(1) << 25)
#define SCTLR_TRE_BIT		(U(1) << 28)
#define SCTLR_AFE_BIT		(U(1) << 29)
#define SCTLR_TE_BIT		(U(1) << 30)
#define SCTLR_DSSBS_BIT		(U(1) << 31)
#define SCTLR_RESET_VAL		(SCTLR_RES1 | SCTLR_NTWE_BIT |		\
				SCTLR_NTWI_BIT | SCTLR_CP15BEN_BIT)

/* SDCR definitions */
#define SDCR_SPD(x)		((x) << 14)
#define SDCR_SPD_LEGACY		U(0x0)
#define SDCR_SPD_DISABLE	U(0x2)
#define SDCR_SPD_ENABLE		U(0x3)
#define SDCR_SCCD_BIT		(U(1) << 23)
#define SDCR_TTRF_BIT		(U(1) << 19)
#define SDCR_SPME_BIT		(U(1) << 17)
#define SDCR_RESET_VAL		U(0x0)
#define SDCR_MTPME_BIT		(U(1) << 28)

/* HSCTLR definitions */
#define HSCTLR_RES1	((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
			 (U(1) << 22) | (U(1) << 18) | (U(1) << 16) | \
			 (U(1) << 11) | (U(1) << 6)  | (U(1) << 5) | (U(1) << 4) | (U(1) << 3))

#define HSCTLR_M_BIT		(U(1) << 0) // MPU [v8-r]
#define HSCTLR_A_BIT		(U(1) << 1)
#define HSCTLR_C_BIT		(U(1) << 2)
#define HSCTLR_CP15BEN_BIT	(U(1) << 5)
#define HSCTLR_ITD_BIT		(U(1) << 7)
#define HSCTLR_SED_BIT		(U(1) << 8)
#define HSCTLR_I_BIT		(U(1) << 12)
#define HSCTLR_BR_BIT		(U(1) << 17) // MPU [v8-r]
#define HSCTLR_WXN_BIT		(U(1) << 19)
#define HSCTLR_EE_BIT		(U(1) << 25)
#define HSCTLR_TE_BIT		(U(1) << 30)
#define HSCTLR_SET 	(HSCTLR_RES1 | HSCTLR_A_BIT)
/* CPACR definitions */
#define CPACR_FPEN(x)		((x) << 20)
#define CPACR_FP_TRAP_PL0	UL(0x1)
#define CPACR_FP_TRAP_ALL	UL(0x2)
#define CPACR_FP_TRAP_NONE	UL(0x3)

/* SCR definitions */
#define SCR_TWE_BIT		(UL(1) << 13)
#define SCR_TWI_BIT		(UL(1) << 12)
#define SCR_SIF_BIT		(UL(1) << 9)
#define SCR_HCE_BIT		(UL(1) << 8)
#define SCR_SCD_BIT		(UL(1) << 7)
#define SCR_NET_BIT		(UL(1) << 6)
#define SCR_AW_BIT		(UL(1) << 5)
#define SCR_FW_BIT		(UL(1) << 4)
#define SCR_EA_BIT		(UL(1) << 3)
#define SCR_FIQ_BIT		(UL(1) << 2)
#define SCR_IRQ_BIT		(UL(1) << 1)
#define SCR_NS_BIT		(UL(1) << 0)
#define SCR_VALID_BIT_MASK	U(0x33ff)
#define SCR_RESET_VAL		U(0x0)

#define GET_NS_BIT(scr)		((scr) & SCR_NS_BIT)

/* HCR definitions */
#define HCR_TGE_BIT		(U(1) << 27)
#define HCR_AMO_BIT		(U(1) << 5)
#define HCR_IMO_BIT		(U(1) << 4)
#define HCR_FMO_BIT		(U(1) << 3)
#define HCR_RESET_VAL		U(0x0)

/* CNTHCTL definitions */
#define CNTHCTL_RESET_VAL	U(0x0)
#define PL1PCEN_BIT		(U(1) << 1)
#define PL1PCTEN_BIT		(U(1) << 0)

/* CNTKCTL definitions */
#define PL0PTEN_BIT		(U(1) << 9)
#define PL0VTEN_BIT		(U(1) << 8)
#define PL0PCTEN_BIT		(U(1) << 0)
#define PL0VCTEN_BIT		(U(1) << 1)
#define EVNTEN_BIT		(U(1) << 2)
#define EVNTDIR_BIT		(U(1) << 3)
#define EVNTI_SHIFT		U(4)
#define EVNTI_MASK		U(0xf)

/* HCPTR definitions */
#define HCPTR_RES1		((U(1) << 13) | (U(1) << 12) | U(0x3ff))
#define TCPAC_BIT		(U(1) << 31)
#define TAM_SHIFT		U(30)
#define TAM_BIT			(U(1) << TAM_SHIFT)
#define TTA_BIT			(U(1) << 20)
#define TASE_BIT		(U(1) << 15)
#define TCP11_BIT		(U(1) << 11)
#define TCP10_BIT		(U(1) << 10)
#define HCPTR_RESET_VAL		HCPTR_RES1
/* HCPTR */
#define HCPTR_MASK      ~(TASE_BIT| TCP11_BIT | TCP10_BIT)

/* HCPTR Hyp. Coprocessor Trap Register */
#define HCPTR_TAM       (U(1)<<30)
#define HCPTR_TTA       (U(1)<<20)        /* Trap trace registers */
#define HCPTR_CP(x)     (U(1)<<(x))       /* Trap Coprocessor x */
#define HCPTR_CP_MASK   ((U(1)<<14)-1)


/* VTTBR defintions */
#define VTTBR_RESET_VAL		ULL(0x0)
#define VTTBR_VMID_MASK		ULL(0xff)
#define VTTBR_VMID_SHIFT	U(48)
#define VTTBR_BADDR_MASK	ULL(0xffffffffffff)
#define VTTBR_BADDR_SHIFT	U(0)

/* HDCR definitions */
#define HDCR_MTPME_BIT		(U(1) << 28)
#define HDCR_HLP_BIT		(U(1) << 26)
#define HDCR_HPME_BIT		(U(1) << 7)
#define HDCR_RESET_VAL		U(0x0)

/* HDCR Hyp. Debug Configuration Register */
#define HDCR_TDRA       (U(1)<<11)          /* Trap Debug ROM access */
#define HDCR_TDOSA      (U(1)<<10)          /* Trap Debug-OS-related register access */
#define HDCR_TDA        (U(1)<<9)           /* Trap Debug Access */
#define HDCR_TDE        (U(1)<<8)           /* Route Soft Debug exceptions from EL1/EL1 to EL2 */
#define HDCR_TPM        (U(1)<<6)           /* Trap Performance Monitors accesses */
#define HDCR_TPMCR      (U(1)<<5)           /* Trap PMCR accesses */

/* HSTR definitions */
#define HSTR_RESET_VAL		U(0x0)
/* HSTR Hyp. System Trap Register */
#define HSTR_T(x)       (U(1)<<(x))       /* Trap Cp15 c<x> */

/* CNTHP_CTL definitions */
#define CNTHP_CTL_RESET_VAL	U(0x0)

/* NSACR definitions */
#define NSASEDIS_BIT		(U(1) << 15)
#define NSTRCDIS_BIT		(U(1) << 20)
#define NSACR_CP11_BIT		(U(1) << 11)
#define NSACR_CP10_BIT		(U(1) << 10)
#define NSACR_IMP_DEF_MASK	(U(0x7) << 16)
#define NSACR_ENABLE_FP_ACCESS	(NSACR_CP11_BIT | NSACR_CP10_BIT)
#define NSACR_RESET_VAL		U(0x0)

/* CPACR definitions */
#define ASEDIS_BIT		(U(1) << 31)
#define TRCDIS_BIT		(U(1) << 28)
#define CPACR_CP11_SHIFT	U(22)
#define CPACR_CP10_SHIFT	U(20)
#define CPACR_ENABLE_FP_ACCESS	((U(0x3) << CPACR_CP11_SHIFT) |\
				 (U(0x3) << CPACR_CP10_SHIFT))
#define CPACR_RESET_VAL		U(0x0)

/* FPEXC definitions */
#define FPEXC_RES1		((U(1) << 10) | (U(1) << 9) | (U(1) << 8))
#define FPEXC_EN_BIT		(U(1) << 30)
#define FPEXC_RESET_VAL		FPEXC_RES1

/* SPSR/CPSR definitions */
#define SPSR_FIQ_BIT		(U(1) << 0)
#define SPSR_IRQ_BIT		(U(1) << 1)
#define SPSR_ABT_BIT		(U(1) << 2)
#define SPSR_AIF_SHIFT		U(6)
#define SPSR_AIF_MASK		U(0x7)

#define SPSR_E_SHIFT		U(9)
#define SPSR_E_MASK		U(0x1)
#define SPSR_E_LITTLE		U(0)
#define SPSR_E_BIG		U(1)

#define SPSR_T_SHIFT		U(5)
#define SPSR_T_MASK		U(0x1)
#define SPSR_T_ARM		U(0)
#define SPSR_T_THUMB		U(1)

#define SPSR_MODE_SHIFT		U(0)
#define SPSR_MODE_MASK		U(0x7)

#define SPSR_SSBS_BIT		BIT_32(23)

#define DISABLE_ALL_EXCEPTIONS \
		(SPSR_FIQ_BIT | SPSR_IRQ_BIT | SPSR_ABT_BIT)

#define CPSR_DIT_BIT		(U(1) << 21)
/*
 * TTBCR definitions
 */
#define TTBCR_EAE_BIT		(U(1) << 31)

#define TTBCR_SH1_NON_SHAREABLE		(U(0x0) << 28)
#define TTBCR_SH1_OUTER_SHAREABLE	(U(0x2) << 28)
#define TTBCR_SH1_INNER_SHAREABLE	(U(0x3) << 28)

#define TTBCR_RGN1_OUTER_NC	(U(0x0) << 26)
#define TTBCR_RGN1_OUTER_WBA	(U(0x1) << 26)
#define TTBCR_RGN1_OUTER_WT	(U(0x2) << 26)
#define TTBCR_RGN1_OUTER_WBNA	(U(0x3) << 26)

#define TTBCR_RGN1_INNER_NC	(U(0x0) << 24)
#define TTBCR_RGN1_INNER_WBA	(U(0x1) << 24)
#define TTBCR_RGN1_INNER_WT	(U(0x2) << 24)
#define TTBCR_RGN1_INNER_WBNA	(U(0x3) << 24)

#define TTBCR_EPD1_BIT		(U(1) << 23)
#define TTBCR_A1_BIT		(U(1) << 22)

#define TTBCR_T1SZ_SHIFT	U(16)
#define TTBCR_T1SZ_MASK		U(0x7)
#define TTBCR_TxSZ_MIN		U(0)
#define TTBCR_TxSZ_MAX		U(7)

#define TTBCR_SH0_NON_SHAREABLE		(U(0x0) << 12)
#define TTBCR_SH0_OUTER_SHAREABLE	(U(0x2) << 12)
#define TTBCR_SH0_INNER_SHAREABLE	(U(0x3) << 12)

#define TTBCR_RGN0_OUTER_NC	(U(0x0) << 10)
#define TTBCR_RGN0_OUTER_WBA	(U(0x1) << 10)
#define TTBCR_RGN0_OUTER_WT	(U(0x2) << 10)
#define TTBCR_RGN0_OUTER_WBNA	(U(0x3) << 10)

#define TTBCR_RGN0_INNER_NC	(U(0x0) << 8)
#define TTBCR_RGN0_INNER_WBA	(U(0x1) << 8)
#define TTBCR_RGN0_INNER_WT	(U(0x2) << 8)
#define TTBCR_RGN0_INNER_WBNA	(U(0x3) << 8)

#define TTBCR_EPD0_BIT		(U(1) << 7)
#define TTBCR_T0SZ_SHIFT	U(0)
#define TTBCR_T0SZ_MASK		U(0x7)

/*
 * HTCR definitions
 */
#define HTCR_RES1			((U(1) << 31) | (U(1) << 23))

#define HTCR_SH0_NON_SHAREABLE		(U(0x0) << 12)
#define HTCR_SH0_OUTER_SHAREABLE	(U(0x2) << 12)
#define HTCR_SH0_INNER_SHAREABLE	(U(0x3) << 12)

#define HTCR_RGN0_OUTER_NC	(U(0x0) << 10)
#define HTCR_RGN0_OUTER_WBA	(U(0x1) << 10)
#define HTCR_RGN0_OUTER_WT	(U(0x2) << 10)
#define HTCR_RGN0_OUTER_WBNA	(U(0x3) << 10)

#define HTCR_RGN0_INNER_NC	(U(0x0) << 8)
#define HTCR_RGN0_INNER_WBA	(U(0x1) << 8)
#define HTCR_RGN0_INNER_WT	(U(0x2) << 8)
#define HTCR_RGN0_INNER_WBNA	(U(0x3) << 8)

#define HTCR_T0SZ_SHIFT		U(0)
#define HTCR_T0SZ_MASK		U(0x7)

/* VSCTLR */
#define VSCTLR_VMID(vmid)	(((vmid) & 0xff) << 16)
#define VSCTLR_S2NIE		(U(0x1) << 2)
#define VSCTLR_S2DMAD		(U(0x1) << 1)

#define MODE_RW_SHIFT		U(0x4)
#define MODE_RW_MASK		U(0x1)
#define MODE_RW_32		U(0x1)

#define MODE32_SHIFT		U(0)
#define MODE32_MASK		U(0x1f)
#define MODE32_usr		U(0x10)
#define MODE32_fiq		U(0x11)
#define MODE32_irq		U(0x12)
#define MODE32_svc		U(0x13)
#define MODE32_mon		U(0x16)
#define MODE32_abt		U(0x17)
#define MODE32_hyp		U(0x1a)
#define MODE32_und		U(0x1b)
#define MODE32_sys		U(0x1f)

#define GET_M32(mode)		(((mode) >> MODE32_SHIFT) & MODE32_MASK)

#define SPSR_MODE32(mode, isa, endian, aif) \
( \
	( \
		(MODE_RW_32 << MODE_RW_SHIFT) | \
		(((mode) & MODE32_MASK) << MODE32_SHIFT) | \
		(((isa) & SPSR_T_MASK) << SPSR_T_SHIFT) | \
		(((endian) & SPSR_E_MASK) << SPSR_E_SHIFT) | \
		(((aif) & SPSR_AIF_MASK) << SPSR_AIF_SHIFT) \
	) & \
	(~(SPSR_SSBS_BIT)) \
)

/*
 * TTBR definitions
 */
#define TTBR_CNP_BIT		ULL(0x1)

/*
 * CTR definitions
 */
#define CTR_CWG_SHIFT		U(24)
#define CTR_CWG_MASK		U(0xf)
#define CTR_ERG_SHIFT		U(20)
#define CTR_ERG_MASK		U(0xf)
#define CTR_DMINLINE_SHIFT	U(16)
#define CTR_DMINLINE_WIDTH	U(4)
#define CTR_DMINLINE_MASK	((U(1) << 4) - U(1))
#define CTR_L1IP_SHIFT		U(14)
#define CTR_L1IP_MASK		U(0x3)
#define CTR_IMINLINE_SHIFT	U(0)
#define CTR_IMINLINE_MASK	U(0xf)

#define MAX_CACHE_LINE_SIZE	U(0x800) /* 2KB */

/* PMCR definitions */
#define PMCR_N_SHIFT		U(11)
#define PMCR_N_MASK		U(0x1f)
#define PMCR_N_BITS		(PMCR_N_MASK << PMCR_N_SHIFT)
#define PMCR_LP_BIT		(U(1) << 7)
#define PMCR_LC_BIT		(U(1) << 6)
#define PMCR_DP_BIT		(U(1) << 5)
#define	PMCR_RESET_VAL		U(0x0)

/*******************************************************************************
 * Definitions of register offsets, fields and macros for CPU system
 * instructions.
 ******************************************************************************/

#define TLBI_ADDR_SHIFT		U(0)
#define TLBI_ADDR_MASK		U(0xFFFFF000)
#define TLBI_ADDR(x)		(((x) >> TLBI_ADDR_SHIFT) & TLBI_ADDR_MASK)

/*******************************************************************************
 * Definitions of register offsets and fields in the CNTCTLBase Frame of the
 * system level implementation of the Generic Timer.
 ******************************************************************************/
#define CNTCTLBASE_CNTFRQ	U(0x0)
#define CNTNSAR			U(0x4)
#define CNTNSAR_NS_SHIFT(x)	(x)

#define CNTACR_BASE(x)		(U(0x40) + ((x) << 2))
#define CNTACR_RPCT_SHIFT	U(0x0)
#define CNTACR_RVCT_SHIFT	U(0x1)
#define CNTACR_RFRQ_SHIFT	U(0x2)
#define CNTACR_RVOFF_SHIFT	U(0x3)
#define CNTACR_RWVT_SHIFT	U(0x4)
#define CNTACR_RWPT_SHIFT	U(0x5)

/*******************************************************************************
 * Definitions of register offsets and fields in the CNTBaseN Frame of the
 * system level implementation of the Generic Timer.
 ******************************************************************************/
/* Physical Count register. */
#define CNTPCT_LO		U(0x0)
/* Counter Frequency register. */
#define CNTBASEN_CNTFRQ		U(0x10)
/* Physical Timer CompareValue register. */
#define CNTP_CVAL_LO		U(0x20)
/* Physical Timer Control register. */
#define CNTP_CTL_		U(0x2c)

/* Physical timer control register bit fields shifts and masks */
#define CNTP_CTL_ENABLE_SHIFT	0
#define CNTP_CTL_IMASK_SHIFT	1
#define CNTP_CTL_ISTATUS_SHIFT	2

#define CNTP_CTL_ENABLE_MASK	U(1)
#define CNTP_CTL_IMASK_MASK	U(1)
#define CNTP_CTL_ISTATUS_MASK	U(1)

/* MAIR macros */
#define MAIR0_ATTR_SET(attr, index)	((attr) << ((index) << U(3)))
#define MAIR1_ATTR_SET(attr, index)	((attr) << (((index) - U(3)) << U(3)))

/* HSR macros */
#define HSREC_SHIFT          26
#define HSREC_MASK           (0x3f << HSREC_SHIFT)
#define HSREC_UNKNOWN        0x00
#define HSREC_WFI            0x01
#define HSREC_SVC            0x11
#define HSREC_HVC            0x12
#define HSREC_SMC            0x13
#define HSREC_PREFETCH_ABORT 0x20
#define HSREC_DATA_ABORT     0x24
#define HSRIL32              (1 << 25)

/* HACTLR macros */
#define HACTLR_CPUACTLR         BIT_32(0)
#define HACTLR_CDBGDCI          BIT_32(1)
#define HACTLR_FLASHIFREGIONR   BIT_32(7)
#define HACTLR_PERIPHPREGIONR   BIT_32(8)
#define HACTLR_QOSR_BIT         BIT_32(9)
#define HACTLR_BUSTIMEOUTR_BIT  BIT_32(10)
#define HACTLR_INTMONR_BIT      BIT_32(12)
#define HACTLR_ERR_BIT          BIT_32(13)

#define HACTLR_INIT (HACTLR_ERR_BIT | HACTLR_INTMONR_BIT | \
		     HACTLR_BUSTIMEOUTR_BIT | HACTLR_QOSR_BIT | \
		     HACTLR_PERIPHPREGIONR | HACTLR_FLASHIFREGIONR | \
		     HACTLR_CDBGDCI | HACTLR_CPUACTLR)

/* System register defines The format is: coproc, opt1, CRn, CRm, opt2 */
#define SCR		p15, 0, c1, c1, 0
#define SCTLR		p15, 0, c1, c0, 0
#define ACTLR		p15, 0, c1, c0, 1
#define HACR		p15, 4, c1, c1, 7
#define HACTLR		p15, 4, c1, c0, 1
#define HADFSR		p15, 4, c5, c1, 0
#define HAIFSR		p15, 4, c5, c1, 1
#define HAMAIR0		p15, 4, c10, c3, 0
#define HAMAIR1		p15, 4, c10, c3, 1
#define SDCR		p15, 0, c1, c3, 1
#define MPIDR		p15, 0, c0, c0, 5
#define MIDR		p15, 0, c0, c0, 0
#define HVBAR		p15, 4, c12, c0, 0
#define VBAR		p15, 0, c12, c0, 0
#define MVBAR		p15, 0, c12, c0, 1
#define NSACR		p15, 0, c1, c1, 2
#define CPACR		p15, 0, c1, c0, 2
#define DCCIMVAC	p15, 0, c7, c14, 1
#define DCCMVAC		p15, 0, c7, c10, 1
#define DCIMVAC		p15, 0, c7, c6, 1
#define DCCISW		p15, 0, c7, c14, 2
#define DCCSW		p15, 0, c7, c10, 2
#define DCISW		p15, 0, c7, c6, 2
#define CTR		p15, 0, c0, c0, 1

#define ADFSR		p15, 0, c5, c1, 0
#define AIFSR		p15, 0, c5, c1, 1
#define CONTEXTIDR	p15, 0, c13, c0, 1


#define ID_AFR0         p15, 0, c0, c1, 3   /* Auxiliary Feature Register 0 */

#define ID_DFR0		p15, 0, c0, c1, 2
#define ID_DFR1		p15, 0, c0, c3, 5
#define ID_PFR0		p15, 0, c0, c1, 0
#define ID_PFR1		p15, 0, c0, c1, 1
#define ID_PFR2         p15,0,c0,c3,4   /* Processor Feature Register 2 */

#define ID_MMFR0        p15,0,c0,c1,4   /* Memory Model Feature Register 0 */
#define ID_MMFR1        p15,0,c0,c1,5   /* Memory Model Feature Register 1 */
#define ID_MMFR2        p15,0,c0,c1,6   /* Memory Model Feature Register 2 */
#define ID_MMFR3        p15,0,c0,c1,7   /* Memory Model Feature Register 3 */
#define ID_MMFR4        p15,0,c0,c2,6   /* Memory Model Feature Register 4 */
#define ID_MMFR5        p15,0,c0,c3,6   /* Memory Model Feature Register 5 */
#define ID_ISAR0        p15,0,c0,c2,0   /* ISA Feature Register 0 */
#define ID_ISAR1        p15,0,c0,c2,1   /* ISA Feature Register 1 */
#define ID_ISAR2        p15,0,c0,c2,2   /* ISA Feature Register 2 */
#define ID_ISAR3        p15,0,c0,c2,3   /* ISA Feature Register 3 */
#define ID_ISAR4        p15,0,c0,c2,4   /* ISA Feature Register 4 */
#define ID_ISAR5        p15,0,c0,c2,5   /* ISA Feature Register 5 */
#define ID_ISAR6        p15,0,c0,c2,7   /* ISA Feature Register 6 */

#define MAIR0		p15, 0, c10, c2, 0
#define MAIR1		p15, 0, c10, c2, 1
#define AMAIR0          p15, 0, c10, c3, 0  /* Aux. Memory Attribute Indirection Register 0 */
#define AMAIR1          p15, 0, c10, c3, 1  /* Aux. Memory Attribute Indirection Register 1 */

#define TTBCR		p15, 0, c2, c0, 2
#define TTBCR2          p15, 0, c2, c0, 3   /* Translation Table Base Control Register 2 */

#define TTBR0		p15, 0, c2, c0, 0
#define TTBR1		p15, 0, c2, c0, 1
#define TTBR0_32		p15, 0, c2, c0, 0
#define TTBR1_32		p15, 0, c2, c0, 1
#define TLBIALL		p15, 0, c8, c7, 0
#define TLBIALLH	p15, 4, c8, c7, 0
#define TLBIALLIS	p15, 0, c8, c3, 0
#define TLBIALLHIS	p15, 4, c8, c3, 0
#define TLBIALLNSNH	p15, 4, c8, c7, 4
#define TLBIALLNSNHIS	p15, 4, c8, c3, 4
#define TLBIMVA		p15, 0, c8, c7, 1
#define TLBIMVAH	p15, 4, c8, c7, 1
#define TLBIMVAIS	P15, 0, c8, c3, 1
#define TLBIMVAA	p15, 0, c8, c7, 3
#define TLBIMVAAIS	p15, 0, c8, c3, 3
#define TLBIMVAHIS	p15, 4, c8, c3, 1
#define BPIALLIS	p15, 0, c7, c1, 6
#define BPIALL		p15, 0, c7, c5, 6
#define ICIALLU		p15, 0, c7, c5, 0
#define HSCTLR		p15, 4, c1, c0, 0
#define HCR		p15, 4, c1, c1, 0
#define HDFAR		p15, 4, c6, c0, 0	
#define HCPTR		p15, 4, c1, c1, 2
#define HSTR		p15, 4, c1, c1, 3
#define VPIDR		p15, 4, c0, c0, 0
#define VMPIDR		p15, 4, c0, c0, 5
#define ISR		p15, 0, c12, c1, 0
#define CLIDR		p15, 1, c0, c0, 1
#define CSSELR		p15, 2, c0, c0, 0
#define CCSIDR		p15, 1, c0, c0, 0
#define CCSIDR2		p15, 1, c0, c0, 2
#define HTCR		p15, 4, c2, c0, 2
#define HMAIR0		p15, 4, c10, c2, 0
#define HMAIR1		p15, 4, c10, c2, 1
#define HPFAR		p15, 4, c6, c0, 4

#define ATS1CPR		p15, 0, c7, c8, 0
#define ATS1CPW		p15, 0, c7, c8, 1
#define ATS1CUR		p15, 0, c7, c8, 2
#define ATS1CUW		p15, 0, c7, c8, 3

#define ATS12NSOPR	p15, 0, c7, c8, 4
#define ATS12NSOPW	p15, 0, c7, c8, 5
#define ATS12NSOUR	p15, 0, c7, c8, 6
#define ATS12NSOUW	p15, 0, c7, c8, 7

#define ATS1HR		p15, 4, c7, c8, 0
#define ATS1HW		p15, 4, c7, c8, 1

#define PAR		p15, 0, c7, c4, 0

#define DBGDIDR         p14,0,c0,c0,0   /* Debug ID Register */
#define DBGDSCRINT      p14,0,c0,c1,0   /* Debug Status and Control Internal */
#define DBGDSCREXT      p14,0,c0,c2,2   /* Debug Status and Control External */
#define DBGVCR          p14,0,c0,c7,0   /* Vector Catch */
#define DBGBVR0         p14,0,c0,c0,4   /* Breakpoint Value 0 */
#define DBGBCR0         p14,0,c0,c0,5   /* Breakpoint Control 0 */
#define DBGWVR0         p14,0,c0,c0,6   /* Watchpoint Value 0 */
#define DBGWCR0         p14,0,c0,c0,7   /* Watchpoint Control 0 */
#define DBGBVR1         p14,0,c0,c1,4   /* Breakpoint Value 1 */
#define DBGBCR1         p14,0,c0,c1,5   /* Breakpoint Control 1 */
#define DBGOSLAR        p14,0,c1,c0,4   /* OS Lock Access */
#define DBGOSLSR        p14,0,c1,c1,4   /* OS Lock Status Register */
#define DBGOSDLR        p14,0,c1,c3,4   /* OS Double Lock */
#define DBGPRCR         p14,0,c1,c4,4   /* Debug Power Control Register */

#define HSR             p15, 4, c5 , c2, 0
#define HIFAR           p15, 4, c6 , c0, 2
#define HTPIDR    	p15, 4, c13, c0, 2
#define TPIDRPRW		p15, 0, c13, c0, 4
#define TPIDRURO		p15, 0, c13, c0, 3
#define TPIDRURW		p15, 0, c13, c0, 3

#define VTCR		p15, 4, c2, c1, 2
/* Debug register defines. The format is: coproc, opt1, CRn, CRm, opt2 */
#define HDCR		p15, 4, c1, c1, 1

/* CP15 CR9: Performance monitors */
#define PMCR            p15,0,c9,c12,0  /* Perf. Mon. Control Register */
#define PMCNTENSET      p15,0,c9,c12,1  /* Perf. Mon. Count Enable Set register */
#define PMCNTENCLR      p15,0,c9,c12,2  /* Perf. Mon. Count Enable Clear register */
#define PMOVSR          p15,0,c9,c12,3  /* Perf. Mon. Overflow Flag Status Register */
#define PMSWINC         p15,0,c9,c12,4  /* Perf. Mon. Software Increment register */
#define PMSELR          p15,0,c9,c12,5  /* Perf. Mon. Event Counter Selection Register */
#define PMCEID0         p15,0,c9,c12,6  /* Perf. Mon. Common Event Identification register 0 */
#define PMCEID1         p15,0,c9,c12,7  /* Perf. Mon. Common Event Identification register 1 */
#define PMCCNTR         p15,0,c9,c13,0  /* Perf. Mon. Cycle Count Register */
#define PMXEVTYPER      p15,0,c9,c13,1  /* Perf. Mon. Event Type Select Register */
#define PMXEVCNTR       p15,0,c9,c13,2  /* Perf. Mon. Event Count Register */
#define PMUSERENR       p15,0,c9,c14,0  /* Perf. Mon. User Enable Register */
#define PMINTENSET      p15,0,c9,c14,1  /* Perf. Mon. Interrupt Enable Set Register */
#define PMINTENCLR      p15,0,c9,c14,2  /* Perf. Mon. Interrupt Enable Clear Register */
#define PMOVSSET        p15,0,c9,c14,3  /* Perf. Mon. Overflow Flag Status Set register */

#define CNTFRQ     	p15, 0, c14, c0, 0/* 32-bit RW Counter Frequency register */
#define CNTPCT_64     	p15, 0, c14   /* 64-bit RO Physical Count register */
#define CNTPTVAL  	p15, 0, c14, c2, 0/* 32-bit RW PL1 Physical TimerValue register */
#define CNTPCTL   	p15, 0, c14, c2, 1/* 32-bit RW PL1 Physical Timer Control register */
#define CNTPCVAL_64  	p15, 2, c14   /* 64-bit RW PL1 Physical Timer CompareValue register */

#define CNTKCTL    	p15, 0, c14, c1, 0/* 32-bit RW Timer PL1 Control register */

#define CNTVCT_64     	p15, 1, c14   /* 64-bit RO Virtual Count register */
#define CNTVTVAL  	p15, 0, c14, c3, 0/* 32-bit RW Virtual TimerValue register */
#define CNTVCTL   	p15, 0, c14, c3, 1/* 32-bit RW Virtual Timer Control register */
#define CNTVCVAL_64  	p15, 3, c14   /* 64-bit RW Virtual Timer CompareValue register */
#define CNTVOFF_64    	p15, 4, c14   /* 64-bit RW Virtual Offset register */

#define CNTHCTL    	p15, 4, c14, c1, 0/* 32-bit RW Timer PL2 Control register */
#define CNTHPTVAL 	p15, 4, c14, c2, 0/* 32-bit RW PL2 Physical TimerValue register */
#define CNTHPCTL  	p15, 4, c14, c2, 1/* 32-bit RW PL2 Physical Timer Control register */
#define CNTHP_CVAL_64 	p15, 6, c14   /* 64-bit RW PL2 Physical Timer CompareValue register */

/* AArch32 coproc registers for 32bit MMU descriptor support */
#define PRRR		p15, 0, c10, c2, 0
#define NMRR		p15, 0, c10, c2, 1
#define DACR		p15, 0, c3, c0, 0

/* GICv3 CPU Interface system register defines. The format is: coproc, opt1, CRn, CRm, opt2 */

#define ICC_IAR0_EL1	p15, 0, c12, c8, 0
#define ICC_EOIR0_EL1	p15, 0, c12, c8, 1
#define ICC_HPPIR0_EL1	p15, 0, c12, c8, 2
#define ICC_BPR0_EL1	p15, 0, c12, c8, 3
#define ICC_RPR_EL1	p15, 0, c12, c11, 3
#define ICC_MCTLR_EL1	p15, 6, c12, c12, 4
#define ICC_HSRE_EL1	p15, 4, c12, c9, 5
#define ICC_MSRE_EL1	p15, 6, c12, c12, 5
#define ICC_IGRPEN0_EL1	p15, 0, c12, c12, 6
#define ICC_MGRPEN1_EL1	p15, 6, c12, c12, 7
#define ICC_IAR1_EL1    p15, 0, c12, c12, 0
#define ICC_EOIR1_EL1   p15, 0, c12, c12, 1
#define ICC_HPPIR1_EL1  p15, 0, c12, c12, 2
#define ICC_BPR1_EL1    p15, 0, c12, c12, 3
#define ICC_DIR_EL1     p15, 0, c12, c11, 1
#define ICC_PMR_EL1     p15, 0, c4, c6, 0
#define ICC_CTLR_EL1    p15, 0, c12, c12, 4
#define ICC_IGRPEN1_EL1 p15, 0, c12, c12, 7
#define ICC_SRE_EL1     p15, 0, c12, c12, 5

/* Virt control registers */
#define ICH_AP0R0_EL2   p15, 4, c12, c8, 0
#define ICH_AP0R1_EL2   p15, 4, c12, c8, 1
#define ICH_AP0R2_EL2   p15, 4, c12, c8, 2
#define ICH_AP0R3_EL2   p15, 4, c12, c8, 3
#define ICH_AP1R0_EL2   p15, 4, c12, c9, 0
#define ICH_AP1R1_EL2   p15, 4, c12, c9, 1
#define ICH_AP1R2_EL2   p15, 4, c12, c9, 2
#define ICH_AP1R3_EL2   p15, 4, c12, c9, 3
#define ICH_HCR_EL2     p15, 4, c12, c11, 0
#define ICH_VTR_EL2     p15, 4, c12, c11, 1
#define ICH_MISR_EL2    p15, 4, c12, c11, 2
#define ICH_EISR_EL2    p15, 4, c12, c11, 3
#define ICH_ELRSR_EL2   p15, 4, c12, c11, 5
#define ICH_VMCR_EL2    p15, 4, c12, c11, 7
#define ICH_LR0_EL2     p15, 4, c12, c12, 0
#define ICH_LR1_EL2     p15, 4, c12, c12, 1
#define ICH_LR2_EL2     p15, 4, c12, c12, 2
#define ICH_LR3_EL2     p15, 4, c12, c12, 3
#define ICH_LR4_EL2     p15, 4, c12, c12, 4
#define ICH_LR5_EL2     p15, 4, c12, c12, 5
#define ICH_LR6_EL2     p15, 4, c12, c12, 6
#define ICH_LR7_EL2     p15, 4, c12, c12, 7
#define ICH_LR8_EL2     p15, 4, c12, c13, 0
#define ICH_LR9_EL2     p15, 4, c12, c13, 1
#define ICH_LR10_EL2    p15, 4, c12, c13, 2
#define ICH_LR11_EL2    p15, 4, c12, c13, 3
#define ICH_LR12_EL2    p15, 4, c12, c13, 4
#define ICH_LR13_EL2    p15, 4, c12, c13, 5
#define ICH_LR14_EL2    p15, 4, c12, c13, 6
#define ICH_LR15_EL2    p15, 4, c12, c13, 7

/* 64 bit system register defines The format is: coproc, opt1, CRm */
#define TTBR0_64	p15, 0, c2
#define TTBR1_64	p15, 1, c2
#define VTTBR_64	p15, 6, c2
#define HTTBR_64	p15, 4, c2
#define PAR_64		p15, 0, c7

/* 64 bit GICv3 CPU Interface system register defines. The format is: coproc, opt1, CRm */
#define ICC_SGI1R_EL1_64	p15, 0, c12
#define ICC_ASGI1R_EL1_64	p15, 1, c12
#define ICC_SGI0R_EL1_64	p15, 2, c12

/* Fault registers. The format is: coproc, opt1, CRn, CRm, opt2 */
#define DFSR		p15, 0, c5, c0, 0
#define IFSR		p15, 0, c5, c0, 1
#define DFAR		p15, 0, c6, c0, 0
#define IFAR		p15, 0, c6, c0, 2

/* Armv8-R MPU registers */
#define HMPUIR		p15, 4, c0, c0, 4
#define HPRBAR		p15, 4, c6, c3, 0
#define HPRENR		p15, 4, c6, c1, 1
#define HPRLAR		p15, 4, c6, c3, 1	
#define HPRSELR		p15, 4, c6, c2, 1
#define MPUIR		p15, 0, c0, c0, 4
#define PRBAR		p15, 0, c6, c3, 0
#define PRLAR		p15, 0, c6, c3, 1
#define PRSELR		p15, 0, c6, c2, 1
#define VSCTLR		p15, 4, c2, c0, 0

/* refined registers */
/*
特别关注这些寄存器的配置与armv7-a的区别
control:
HCR  -- ok(删除SMC和TLB相关)
HCR2 -- dont care

SCTLR -- ok(增加MPU background region相关)
HSCTLR -- ok(增加MPU background region相关)

exception:
HSR  -- ok(删除了一些MMU相关的，增加MPU相关的，但总体还是ARM_FSR_LONG_FORMAT)
IFSR -- ok 内容包含在HSR内，很少使用
DFSR -- ok 内容包含在HSR内，很少使用
PAR -- ok 在地址转换中使用，包含一些调试信息，暂时不用

ignore:暂时不用
HCPTR
PMCR
ID_MMFR0
ID_MMFR2
HDCR
DBGDSCRext
DBGAUTHSTATUS

*/


/* CP14 CR0: */
#define TEECR           p14,6,c0,c0,0   /* ThumbEE Configuration Register */
#define TEEHBR          p14,6,c1,c0,0   /* ThumbEE Handler Base Register */
#define MVFR0           p10,7,c7,c0,0   /* Media and VFP Feature Register 0 */
#define MVFR1           p10,7,c6,c0,0   /* Media and VFP Feature Register 1 */
#define MVFR2           p10,7,c5,c0,0   /* Media and VFP Feature Register 2 */

/* Aliases of AArch64 names for use in common code when building for AArch32 */

/* Alphabetically... */
#define ACTLR_EL1               ACTLR
#define AFSR0_EL1               ADFSR
#define AFSR1_EL1               AIFSR
#define CCSIDR_EL1              CCSIDR
#define CLIDR_EL1               CLIDR
#define CNTFRQ_EL0              CNTFRQ
#define CNTHCTL_EL2             CNTHCTL
#define CNTHP_CTL_EL2           CNTHPCTL
#define CNTHP_CVAL_EL2          CNTHP_CVAL_64
#define CNTKCTL_EL1             CNTKCTL
#define CNTPCT_EL0              CNTPCT_64
#define CNTP_CTL_EL0            CNTP_CTL
#define CNTP_CVAL_EL0           CNTPCVAL_64
#define CNTVCT_EL0              CNTVCT_64
#define CNTVOFF_EL2             CNTVOFF_64
#define CNTV_CTL_EL0            CNTVCTL
#define CNTV_CVAL_EL0           CNTVCVAL_64
#define CONTEXTIDR_EL1          CONTEXTIDR
#define CPACR_EL1               CPACR
#define CPTR_EL2                HCPTR
#define CSSELR_EL1              CSSELR
#define CTR_EL0                 CTR
#define DACR32_EL2              DACR
#define ESR_EL1                 DFSR
#define ESR_EL2                 HSR
#define HCR_EL2                 HCR
#define HPFAR_EL2               HPFAR
#define HSTR_EL2                HSTR
#define ID_AFR0_EL1             ID_AFR0
#define ID_DFR0_EL1             ID_DFR0
#define ID_DFR1_EL1             ID_DFR1
#define ID_ISAR0_EL1            ID_ISAR0
#define ID_ISAR1_EL1            ID_ISAR1
#define ID_ISAR2_EL1            ID_ISAR2
#define ID_ISAR3_EL1            ID_ISAR3
#define ID_ISAR4_EL1            ID_ISAR4
#define ID_ISAR5_EL1            ID_ISAR5
#define ID_ISAR6_EL1            ID_ISAR6
#define ID_MMFR0_EL1            ID_MMFR0
#define ID_MMFR1_EL1            ID_MMFR1
#define ID_MMFR2_EL1            ID_MMFR2
#define ID_MMFR3_EL1            ID_MMFR3
#define ID_MMFR4_EL1            ID_MMFR4
#define ID_MMFR5_EL1            ID_MMFR5
#define ID_PFR0_EL1             ID_PFR0
#define ID_PFR1_EL1             ID_PFR1
#define ID_PFR2_EL1             ID_PFR2
#define IFSR32_EL2              IFSR
#define MDCR_EL2                HDCR
#define MIDR_EL1                MIDR
#define MPIDR_EL1               MPIDR
#define PAR_EL1                 PAR
#define SCTLR_EL1               SCTLR
#define SCTLR_EL2               HSCTLR
#define TCR_EL1                 TTBCR
#define TEECR32_EL1             TEECR
#define TEEHBR32_EL1            TEEHBR
#define TPIDRRO_EL0             TPIDRURO
#define TPIDR_EL0               TPIDRURW
#define TPIDR_EL1               TPIDRPRW
#define TPIDR_EL2               HTPIDR
#define TTBR0_EL1               TTBR0_64
#define TTBR0_EL2               HTTBR_64
#define TTBR1_EL1               TTBR1_64
#define VBAR_EL1                VBAR
#define VBAR_EL2                HVBAR
#define VMPIDR_EL2              VMPIDR
#define VPIDR_EL2               VPIDR
#define VTCR_EL2                VTCR
#define VTTBR_EL2               VTTBR_64
#define MVFR0_EL1               MVFR0
#define MVFR1_EL1               MVFR1
#define MVFR2_EL1               MVFR2


/*******************************************************************************
 * Definitions of MAIR encodings for device and normal memory
 ******************************************************************************/
/*
 * MAIR encodings for device memory attributes.
 */
#define MAIR_DEV_nGnRnE		U(0x0)
#define MAIR_DEV_nGnRE		U(0x4)
#define MAIR_DEV_nGRE		U(0x8)
#define MAIR_DEV_GRE		U(0xc)

/*
 * MAIR encodings for normal memory attributes.
 *
 * Cache Policy
 *  WT:	 Write Through
 *  WB:	 Write Back
 *  NC:	 Non-Cacheable
 *
 * Transient Hint
 *  NTR: Non-Transient
 *  TR:	 Transient
 *
 * Allocation Policy
 *  RA:	 Read Allocate
 *  WA:	 Write Allocate
 *  RWA: Read and Write Allocate
 *  NA:	 No Allocation
 */
#define MAIR_NORM_WT_TR_WA	U(0x1)
#define MAIR_NORM_WT_TR_RA	U(0x2)
#define MAIR_NORM_WT_TR_RWA	U(0x3)
#define MAIR_NORM_NC		U(0x4)
#define MAIR_NORM_WB_TR_WA	U(0x5)
#define MAIR_NORM_WB_TR_RA	U(0x6)
#define MAIR_NORM_WB_TR_RWA	U(0x7)
#define MAIR_NORM_WT_NTR_NA	U(0x8)
#define MAIR_NORM_WT_NTR_WA	U(0x9)
#define MAIR_NORM_WT_NTR_RA	U(0xa)
#define MAIR_NORM_WT_NTR_RWA	U(0xb)
#define MAIR_NORM_WB_NTR_NA	U(0xc)
#define MAIR_NORM_WB_NTR_WA	U(0xd)
#define MAIR_NORM_WB_NTR_RA	U(0xe)
#define MAIR_NORM_WB_NTR_RWA	U(0xf)

#define MAIR_NORM_OUTER_SHIFT	U(4)

#define MAKE_MAIR_NORMAL_MEMORY(inner, outer)	\
		((inner) | ((outer) << MAIR_NORM_OUTER_SHIFT))

/* PAR fields */
#define PAR_F_SHIFT	U(0)
#define PAR_F_MASK	ULL(0x1)
#define PAR_ADDR_SHIFT	U(12)
#define PAR_ADDR_MASK	(BIT_64(40) - ULL(1)) /* 40-bits-wide page address */


/*******************************************************************************
 * Definitions for system register interface to AMU for FEAT_AMUv1
 ******************************************************************************/
#define AMCR		p15, 0, c13, c2, 0
#define AMCFGR		p15, 0, c13, c2, 1
#define AMCGCR		p15, 0, c13, c2, 2
#define AMUSERENR	p15, 0, c13, c2, 3
#define AMCNTENCLR0	p15, 0, c13, c2, 4
#define AMCNTENSET0	p15, 0, c13, c2, 5
#define AMCNTENCLR1	p15, 0, c13, c3, 0
#define AMCNTENSET1	p15, 0, c13, c3, 1

/* Activity Monitor Group 0 Event Counter Registers */
#define AMEVCNTR00	p15, 0, c0
#define AMEVCNTR01	p15, 1, c0
#define AMEVCNTR02	p15, 2, c0
#define AMEVCNTR03	p15, 3, c0

/* Activity Monitor Group 0 Event Type Registers */
#define AMEVTYPER00	p15, 0, c13, c6, 0
#define AMEVTYPER01	p15, 0, c13, c6, 1
#define AMEVTYPER02	p15, 0, c13, c6, 2
#define AMEVTYPER03	p15, 0, c13, c6, 3

/* Activity Monitor Group 1 Event Counter Registers */
#define AMEVCNTR10	p15, 0, c4
#define AMEVCNTR11	p15, 1, c4
#define AMEVCNTR12	p15, 2, c4
#define AMEVCNTR13	p15, 3, c4
#define AMEVCNTR14	p15, 4, c4
#define AMEVCNTR15	p15, 5, c4
#define AMEVCNTR16	p15, 6, c4
#define AMEVCNTR17	p15, 7, c4
#define AMEVCNTR18	p15, 0, c5
#define AMEVCNTR19	p15, 1, c5
#define AMEVCNTR1A	p15, 2, c5
#define AMEVCNTR1B	p15, 3, c5
#define AMEVCNTR1C	p15, 4, c5
#define AMEVCNTR1D	p15, 5, c5
#define AMEVCNTR1E	p15, 6, c5
#define AMEVCNTR1F	p15, 7, c5

/* Activity Monitor Group 1 Event Type Registers */
#define AMEVTYPER10	p15, 0, c13, c14, 0
#define AMEVTYPER11	p15, 0, c13, c14, 1
#define AMEVTYPER12	p15, 0, c13, c14, 2
#define AMEVTYPER13	p15, 0, c13, c14, 3
#define AMEVTYPER14	p15, 0, c13, c14, 4
#define AMEVTYPER15	p15, 0, c13, c14, 5
#define AMEVTYPER16	p15, 0, c13, c14, 6
#define AMEVTYPER17	p15, 0, c13, c14, 7
#define AMEVTYPER18	p15, 0, c13, c15, 0
#define AMEVTYPER19	p15, 0, c13, c15, 1
#define AMEVTYPER1A	p15, 0, c13, c15, 2
#define AMEVTYPER1B	p15, 0, c13, c15, 3
#define AMEVTYPER1C	p15, 0, c13, c15, 4
#define AMEVTYPER1D	p15, 0, c13, c15, 5
#define AMEVTYPER1E	p15, 0, c13, c15, 6
#define AMEVTYPER1F	p15, 0, c13, c15, 7

/* AMCNTENSET0 definitions */
#define AMCNTENSET0_Pn_SHIFT	U(0)
#define AMCNTENSET0_Pn_MASK	U(0xffff)

/* AMCNTENSET1 definitions */
#define AMCNTENSET1_Pn_SHIFT	U(0)
#define AMCNTENSET1_Pn_MASK	U(0xffff)

/* AMCNTENCLR0 definitions */
#define AMCNTENCLR0_Pn_SHIFT	U(0)
#define AMCNTENCLR0_Pn_MASK	U(0xffff)

/* AMCNTENCLR1 definitions */
#define AMCNTENCLR1_Pn_SHIFT	U(0)
#define AMCNTENCLR1_Pn_MASK	U(0xffff)

/* AMCR definitions */
#define AMCR_CG1RZ_SHIFT	U(17)
#define AMCR_CG1RZ_BIT		(ULL(1) << AMCR_CG1RZ_SHIFT)

/* AMCFGR definitions */
#define AMCFGR_NCG_SHIFT	U(28)
#define AMCFGR_NCG_MASK		U(0xf)
#define AMCFGR_N_SHIFT		U(0)
#define AMCFGR_N_MASK		U(0xff)

/* AMCGCR definitions */
#define AMCGCR_CG0NC_SHIFT	U(0)
#define AMCGCR_CG0NC_MASK	U(0xff)
#define AMCGCR_CG1NC_SHIFT	U(8)
#define AMCGCR_CG1NC_MASK	U(0xff)

/*******************************************************************************
 * Definitions for DynamicIQ Shared Unit registers
 ******************************************************************************/
#define CLUSTERPWRDN	p15, 0, c15, c3, 6

/* CLUSTERPWRDN register definitions */
#define DSU_CLUSTER_PWR_OFF	0
#define DSU_CLUSTER_PWR_ON	1
#define DSU_CLUSTER_PWR_MASK	U(1)


#define COMPILER_BARRIER() __asm__ volatile("" ::: "memory")
