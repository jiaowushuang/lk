#pragma once

#include <sys/types.h>

#include <kern/compiler.h>

__BEGIN_CDECLS

void *memdup_user_nul(const void __user *src, size_t len);
void dump_objects(status_t ret);
void dump_pages(status_t ret);
void *get_pages(int order);
void *kzalloc(size_t size);
int kfree(void *object);
void *kmalloc(size_t size);

__END_CDECLS


