/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 * Copyright (c) 2013, NVIDIA CORPORATION. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <kern/debug.h>
#include <kern/err.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <root/mm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <platform.h>
#include <root/uthread.h>
#include <root/uio.h>
#include <root/sys_fd.h>
#include <root/app.h>

static int32_t sys_std_write(uint32_t fd, user_addr_t user_ptr, uint32_t size);
static int32_t sys_std_writev(uint32_t fd, user_addr_t iov_uaddr, uint32_t iov_cnt);
static mutex_t fd_lock = MUTEX_INITIAL_VALUE(fd_lock);

static const struct sys_fd_ops sys_std_fd_op = {
#ifdef LIBC_STDIO	
	.write = sys_std_write,
#else
	.write = sys_std_writev,
#endif
};

static struct sys_fd_ops const *sys_fds[MAX_SYS_FD_HADLERS] = {
	[1] = &sys_std_fd_op,  /* stdout */
	[2] = &sys_std_fd_op,  /* stderr */
};

status_t install_sys_fd_handler(uint32_t fd, const struct sys_fd_ops *ops)
{
	status_t ret;

	if (fd >= countof(sys_fds))
		return ERR_INVALID_ARGS;

	mutex_acquire(&fd_lock);
	if (!sys_fds[fd]) {
		sys_fds[fd] = ops;
		ret = NO_ERROR;
	} else {
		ret = ERR_ALREADY_EXISTS;
	}
	mutex_release(&fd_lock);
	return ret;
}

static const struct sys_fd_ops *get_sys_fd_handler(uint32_t fd)
{
	if (fd >= countof(sys_fds))
		return NULL;

	return sys_fds[fd];
}

static bool valid_address(vaddr_t addr, u_int size)
{
	vmm_region_t * region = vmm_find_region(uthread_get_current()->thread->aspace, addr);
	if (region && region->size == size)
		return true;
	return false;
}

/* handle stdout/stderr */
static int32_t sys_std_write(uint32_t fd, user_addr_t user_ptr, uint32_t size)
{
	/* check buffer is in task's address space */
	if (!valid_address((vaddr_t)user_ptr, size))
		return ERR_INVALID_ARGS;
	fwrite((const void *)(uintptr_t)user_ptr, size, 1, (fd == 2) ? stderr : stdout);
//	dwrite((fd == 2) ? INFO : SPEW, (const void *)(uintptr_t)user_ptr, size);
	return size;
}

/* handle stdout/stderr */
static int32_t sys_std_writev(uint32_t fd,
                              user_addr_t iov_uaddr,
                              uint32_t iov_cnt) 
{
    /*
     * Even if we're suppressing the output, we need to process the data to
     * produce the correct return code.
     */
    bool should_output = INFO <= SYS_DEBUGLEVEL;

    io_handle_t* io_handle = fd_io_handle(fd);
    if (io_handle == NULL) {
        return ERR_BAD_HANDLE;
    }
    uint8_t buf[128];

    if (should_output) {
        io_lock(io_handle);
    }

    struct iovec_iter iter = iovec_iter_create(iov_cnt);
    size_t total_bytes = 0;
    int ret;

    while (iovec_iter_has_next(&iter)) {
        ret = user_iovec_to_membuf_iter(buf, sizeof(buf), iov_uaddr, &iter);
        if (ret < 0) {
            goto write_done;
        }
        total_bytes += ret;
        if (should_output) {
            ret = io_write(io_handle, (const void*)buf, ret);
            if (ret < 0) {
                goto write_done;
            }
        }
    }
    ret = total_bytes;

write_done:
    if (should_output) {
        io_write_commit(io_handle);
        io_unlock(io_handle);
    }
    return ret;
}


long sys_write(uint32_t fd, user_addr_t user_ptr, uint32_t size)
{
	const struct sys_fd_ops *ops = get_sys_fd_handler(fd);

	if (ops && ops->write)
		return ops->write(fd, user_ptr, size);

	return ERR_NOT_SUPPORTED;
}

long sys_brk(u_int brk)
{
	app_t *app = uthread_get_current()->private_data;

	/* update brk, if within range */
	if ((brk >= app->start_brk) && (brk <= app->end_brk)) {
		app->cur_brk = brk;
	}
	return (long) app->cur_brk;
}

long sys_exit_group(void)
{
	thread_t *current = get_current_thread();
	dprintf(CRITICAL, "exit called, thread %p, name %s\n",
		current, current->name);
	uthread_exit(0);
	return 0L;
}

long sys_exit_etc(int32_t status, uint32_t flags) {
    	thread_t* current = get_current_thread();
	dprintf(CRITICAL, "exit called, thread %p, name %s\n",
		current, current->name);
    	uthread_exit(status);
    	return 0L;
}

long sys_read(uint32_t fd, user_addr_t user_ptr, uint32_t size)
{
	const struct sys_fd_ops *ops = get_sys_fd_handler(fd);

	if (ops && ops->read)
		return ops->read(fd, user_ptr, size);

	return ERR_NOT_SUPPORTED;
}

long sys_ioctl(uint32_t fd, uint32_t req, user_addr_t user_ptr)
{
	const struct sys_fd_ops *ops = get_sys_fd_handler(fd);

	if (ops && ops->ioctl)
		return ops->ioctl(fd, req, user_ptr);

	return ERR_NOT_SUPPORTED;
}

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

long sys_nanosleep(uint32_t clock_id, uint32_t flags,
		   uint32_t sleep_time_l, uint32_t sleep_time_h)
{
	uint64_t sleep_time = sleep_time_l + ((uint64_t)sleep_time_h << 32);
	sys_time_t sleep_tick = (sys_time_t)(DIV_ROUND_UP(sleep_time, 1000 * 1000));

	thread_sleep(sleep_tick);

	return NO_ERROR;
}

long sys_gettime(uint32_t clock_id, uint32_t flags, user_addr_t time)
{
	// return time in nanoseconds
	sys_bigtime_t t = current_time_hires() * 1000;

	return copy_to_user(time, &t, sizeof(int64_t));
}

long sys_mmap(user_addr_t uaddr, uint32_t size, uint32_t flags, uint32_t handle)
{
	app_t *app = uthread_get_current()->private_data;
	vaddr_t vaddr;
	long ret;

	/*
	 * Only allows mapping on IO region specified by handle (id) and uaddr
	 * must be 0 for now.
	 * TBD: Add support in uthread_map to use uaddr as a hint.
	 */
	if (flags != MMAP_FLAG_IO_HANDLE || uaddr != 0)
		return ERR_INVALID_ARGS;

	ret = app_setup_mmio(app, handle, &vaddr, size);
	if (ret != NO_ERROR)
		return ret;

	return vaddr;
}

long sys_munmap(user_addr_t uaddr, uint32_t size)
{
	app_t *app = uthread_get_current()->private_data;

	/*
	 * vmm_free_region always unmaps whole region.
	 * TBD: Add support to unmap partial region when there's use case.
	 */
	return vmm_free_region(app->ut->thread->aspace, uaddr);
}

#if UTHREAD_WITH_MEMORY_MAPPING_SUPPORT

long sys_prepare_dma(user_addr_t uaddr, uint32_t size, uint32_t flags,
		user_addr_t pmem)
{
	uthread_t *current = uthread_get_current();
	struct dma_pmem kpmem;
	uint32_t mapped_size = 0;
	uint32_t entries = 0;
	long ret;

	if (size == 0 || !valid_address((vaddr_t)uaddr, size))
		return ERR_INVALID_ARGS;

	do {
		ret = uthread_virt_to_phys(current,
				(vaddr_t)uaddr + mapped_size, &kpmem.paddr);
		if (ret != NO_ERROR)
			return ret;

		kpmem.size = MIN(size - mapped_size,
			PAGE_SIZE - (kpmem.paddr & (PAGE_SIZE - 1)));

		ret = copy_to_user(pmem, &kpmem, sizeof(struct dma_pmem));
		if (ret != NO_ERROR)
			return ret;

		pmem += sizeof(struct dma_pmem);

		mapped_size += kpmem.size;
		entries++;

	} while (mapped_size < size && (flags & DMA_FLAG_MULTI_PMEM));

	if (flags & DMA_FLAG_FROM_DEVICE)
		arch_clean_invalidate_cache_range(uaddr, mapped_size);
	else
		arch_clean_cache_range(uaddr, mapped_size);

	if (!(flags & DMA_FLAG_ALLOW_PARTIAL) && mapped_size != size)
		return ERR_BAD_LEN;

	return entries;
}

long sys_finish_dma(user_addr_t uaddr, uint32_t size, uint32_t flags)
{
	/* check buffer is in task's address space */
	if (!valid_address((vaddr_t)uaddr, size))
		return ERR_INVALID_ARGS;

	if (flags & DMA_FLAG_FROM_DEVICE)
		arch_clean_invalidate_cache_range(uaddr, size);

	return NO_ERROR;
}

#else /* !UTHREAD_WITH_MEMORY_MAPPING_SUPPORT */

long sys_prepare_dma(user_addr_t uaddr, uint32_t size, uint32_t flags,
		user_addr_t pmem)
{
	return ERR_NOT_SUPPORTED;
}

long sys_finish_dma(user_addr_t uaddr, uint32_t size, uint32_t flags)
{
	return ERR_NOT_SUPPORTED;
}

long sys_set_user_tls(user_addr_t uaddr) {
    arch_set_user_tls(uaddr);
    return NO_ERROR;
}

#endif
