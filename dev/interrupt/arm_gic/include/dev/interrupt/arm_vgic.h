#ifdef WITH_HYPER_MODE

#if ARM_GIC_VERSION == 2
#include <dev/interrupt/vgic_v2.h>
#endif

#if ARM_GIC_VERSION == 3
#include <dev/interrupt/vgic_v3.h>
#endif

#endif