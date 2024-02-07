armv7-a, armv8-r(32)
armv8-a(32)与armv7-a(armv8-a aarch32)差异性：
ARM Architecture Reference Manual Supplement®ARMv8, for the ARMv8-R AArch32 architecture profile

VMSAv7 with Virtualization Extensions
The implementation supports all of the MMUs, as follows:
Secure PL1&0 stage 1 MMU
        Operation of this MMU can be split between two sets of translation tables, which are
        defined by the Secure copies of TTBR0 and TTBR1, and controlled by the Secure copy
        of TTBCR.
Non-secure PL2 stage 1 MMU
        The HTTBR defines the translation table for this MMU, controlled by HTCR.
Non-secure PL1&0 stage 1 MMU
        Operation of this MMU can be split between two sets of translation tables, which are
        defined by the Non-secure copies of TTBR0 and TTBR1 and controlled by the
        Non-secure copy of TTBCR.
Non-secure PL1&0 stage 2 control
        The VTTBR defines the translation table for this MMU, controlled by VTCR.

MPU类比：
1. Secure PL1&0 stage 1 MMU -- NO Support
2. Non-secure PL2 stage 1 MMU -- EL2 translation regime(EL2 MPU part1)
3. Non-secure PL1&0 stage 1 MMU -- EL1&0 translation regime stage1 translation(EL1 MPU)
4. Non-secure PL1&0 stage 2 control -- EL1&0 translation regime stage2 translation(EL2 MPU part2)

所以，在HYP使用MPU的情况下，EL2 translation regime(EL2 MPU part1)是必须实现的；
然后，根据RTOS的需求，可以分成三个情况：
1. RTOS不使用MPU -- HCR.TGE==1 --> vcpu disabled --> SCTLR.M==0
2. RTOS使用MPU，但不需要stage2 -- HCR.VM==0
3. RTOS使用MPU，需要stage2 -- HCR.VM==1
当然，HYP可以选择不使用MPU

Xen是非常丰富的素材库，可以引用，考察了很多别的Hypervisor，基本都有向Xen取经的