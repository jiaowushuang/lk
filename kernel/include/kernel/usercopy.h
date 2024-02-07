#include <arch.h>

static inline status_t copy_from_user(void *kdest, user_addr_t usrc, user_size_t len)
{
	return arch_copy_from_user(kdest, usrc, len);
}

static inline status_t copy_to_user(user_addr_t udest, const void *ksrc, user_size_t len)
{
	return arch_copy_to_user(udest, ksrc, len);
}

static inline ssize_t strncpy_from_user(char *kdest, user_addr_t usrc, user_size_t len)
{
	/* wrapper for now, the old strncpy_from_user was closer to strlcpy than
	 * strncpy behaviour, but could return an unterminated string */
	return arch_strlcpy_from_user(kdest, usrc, len);
}