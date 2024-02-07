/* The unprivileged Load unprivileged and Store 
 * unprivileged instructions LDRT , LDRSHT , LDRHT ,
 * LDRBT , STRT , STRHT , and STRBT , are UNPREDICTABLE
 * if executed in Hyp mode. 
*/

#include <kern/err.h>
#include <string.h>
#include <arch/arm/virt/mmu.h>
#include <arch/mmu.h>
#include <kernel/vm.h>

status_t arch_copy_to_user(user_addr_t udest, const void *ksrc, size_t len)
{
	status_t ret;
	paddr_t pa;
	void *kvaddr;

	ret = arch_mmu_at_kernel(udest, &pa, ARCH_MMU_AT_FLAG_IPA);
        if (ret)
		return ERR_NOT_FOUND;
	kvaddr = paddr_to_kvaddr(pa);
	memcpy(kvaddr, ksrc, len);
	return NO_ERROR;
}

status_t arch_copy_from_user(void *kdest, user_addr_t usrc, size_t len)
{
	status_t ret;
	paddr_t pa;
	void *kvaddr;

        ret = arch_mmu_at_kernel(usrc, &pa, ARCH_MMU_AT_FLAG_IPA);
        if (ret)
		return ERR_NOT_FOUND;
        kvaddr = paddr_to_kvaddr(pa);
	memcpy(kdest, kvaddr, len);
	return NO_ERROR;        
}

ssize_t arch_strlcpy_from_user(char *kdst, user_addr_t usrc, size_t len)
{
	status_t ret;
	paddr_t pa;
	char *kvaddr;

        ret = arch_mmu_at_kernel(usrc, &pa, ARCH_MMU_AT_FLAG_IPA);
        if (ret)
		return ERR_NOT_FOUND;
        kvaddr = paddr_to_kvaddr(pa);
	memcpy(kdst, kvaddr, len);
	return NO_ERROR;
}