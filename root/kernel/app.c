/*
 * Copyright (c) 2012-2013, NVIDIA CORPORATION. All rights reserved
 * Copyright (c) 2013, Google, Inc. All rights reserved
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

#define DEBUG_LOAD_APP	1

#include <arch.h>
#include <assert.h>
#include <kern/compiler.h>
#include <kern/debug.h>
#include "elf.h"
#include <kern/err.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <malloc.h>
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <root/uthread.h>
#include <kern/init.h>

#include <root/app.h>

/*
 * Layout of .app.manifest section in the trusted application is the
 * required UUID followed by an abitrary number of configuration options.
 *
 * Note: Ensure that the manifest definition is kept in sync with the
 * one userspace uses to build the trusty apps.
 */

enum {
	APP_CONFIG_KEY_MIN_STACK_SIZE	= 1,
	APP_CONFIG_KEY_MIN_HEAP_SIZE	= 2,
	APP_CONFIG_KEY_MAP_MEM		= 3,
};

typedef struct app_manifest {
	uuid_t		uuid;
	char   		app_name[256];
	uint32_t	config_options[];
} app_manifest_t;

#define MAX_APP_COUNT	(PAGE_SIZE / sizeof(app_t))

#define APP_START_ADDR	USER_ASPACE_BASE // 0x8000 /* LD script .text start */
#define APP_STACK_TOP	USER_ASPACE_BASE+0x1000000 /* 16MB */

#define PAGE_MASK		(PAGE_SIZE - 1)

#undef ELF_64BIT
#if !IS_64BIT || USER_32BIT
#define ELF_64BIT 0
#else
#define ELF_64BIT 1
#endif

#if ELF_64BIT
#define ELF_NHDR Elf64_Nhdr
#define ELF_SHDR Elf64_Shdr
#define ELF_EHDR Elf64_Ehdr
#define ELF_PHDR Elf64_Phdr
#define Elf_Addr Elf64_Addr
#define Elf_Off Elf64_Off
#define Elf_Word Elf64_Word

#define PRIxELF_Off PRIx64
#define PRIuELF_Size PRIu64
#define PRIxELF_Size PRIx64
#define PRIxELF_Addr PRIx64
#define PRIxELF_Flags PRIx64
#else
#define ELF_NHDR Elf32_Nhdr
#define ELF_SHDR Elf32_Shdr
#define ELF_EHDR Elf32_Ehdr
#define ELF_PHDR Elf32_Phdr
#define Elf_Addr Elf32_Addr
#define Elf_Off Elf32_Off
#define Elf_Word Elf32_Word

#define PRIxELF_Off PRIx32
#define PRIuELF_Size PRIu32
#define PRIxELF_Size PRIx32
#define PRIxELF_Addr PRIx32
#define PRIxELF_Flags PRIx32
#endif

static u_int app_count;
static app_t *app_list;

static char *app_image_start;
static char *app_image_end;
static u_int app_image_size;

extern intptr_t __app_start;
extern intptr_t __app_end;

static bool apps_started;
static mutex_t apps_lock = MUTEX_INITIAL_VALUE(apps_lock);
static struct list_node app_notifier_list = LIST_INITIAL_VALUE(app_notifier_list);
uint als_slot_cnt;

#define PRINT_APP_UUID(tid,u)					\
	dprintf(SPEW,							\
		"app %d uuid: 0x%x 0x%x 0x%x 0x%x%x 0x%x%x%x%x%x%x\n",\
		tid,							\
		(u)->time_low, (u)->time_mid,				\
		(u)->time_hi_and_version,				\
		(u)->clock_seq_and_node[0],				\
		(u)->clock_seq_and_node[1],				\
		(u)->clock_seq_and_node[2],				\
		(u)->clock_seq_and_node[3],				\
		(u)->clock_seq_and_node[4],				\
		(u)->clock_seq_and_node[5],				\
		(u)->clock_seq_and_node[6],				\
		(u)->clock_seq_and_node[7]);

/*
 * Allocate space on the user stack.
 */
static user_addr_t user_stack_alloc(uthread_t* thread,
                                    user_size_t data_len,
                                    user_size_t align,
                                    user_addr_t* stack_ptr) {
    user_addr_t ptr = ROUNDDOWN(*stack_ptr - data_len, align);

    if (ptr < thread->start_stack - PAGE_SIZE) {
        panic("stack %lx underflow while initializing user space\n", ptr);
    }
    *stack_ptr = ptr;
    return ptr;
}

/*
 * Copy data to a preallocated spot on the user stack. This should not fail.
 */
static void copy_to_user_stack(user_addr_t dst_ptr,
                               const void* data,
                               user_size_t data_len) {
    int ret = copy_to_user(dst_ptr, data, data_len);   
    if (ret) {
        panic("copy_to_user failed %d, dst addr:%lx, data addr:%p, data len:%lx\n", 
		ret, dst_ptr, data, data_len);
    }
}

/*
 * Allocate space on the user stack and fill it with data.
 */
static user_addr_t add_to_user_stack(uthread_t* thread,
                                     const void* data,
                                     user_size_t data_len,
                                     user_size_t align,
                                     user_addr_t* stack_ptr) {
    user_addr_t ptr =
            user_stack_alloc(thread, data_len, align, stack_ptr);
    copy_to_user_stack(ptr, data, data_len);
    return ptr;
}

/* TODO share a common header file. */
#define AT_PAGESZ 6
#define AT_BASE 7
#define AT_RANDOM 25
#define AT_HWCAP2 26
#define HWCAP2_MTE (1 << 18)

/*
 * Pass data to libc on the user stack.
 * Prevent inlining so that the stack allocations inside this function don't get
 * trapped on the kernel stack.
 */
user_addr_t
thread_write_elf_tables(void* image,
                               user_addr_t stack) {
    /* Construct the elf tables in reverse order - the stack grows down. */

    /*
     * sixteen random bytes
     */ 
    app_t *app = (app_t *)image;
    uthread_t *thread = app->ut;
    vaddr_t load_bias = app->load_bias;
    user_addr_t stack_ptr = stack;
    uint8_t rand_bytes[16] = {0};

    // rand_get_bytes(rand_bytes, sizeof(rand_bytes));
    user_addr_t rand_bytes_addr = add_to_user_stack(
            thread, rand_bytes, sizeof(rand_bytes), 1, &stack_ptr);

    const char* app_name = app->app_name;
    user_addr_t app_name_addr =
            add_to_user_stack(thread, app_name, strlen(app_name) + 1,
                              sizeof(user_addr_t), &stack_ptr);

    bool mte = false; //arch_tagging_enabled();
    /* auxv */
    user_addr_t auxv[] = {
            AT_PAGESZ, PAGE_SIZE,       AT_BASE,   load_bias,
            AT_RANDOM, rand_bytes_addr, AT_HWCAP2, mte ? HWCAP2_MTE : 0,
            0};
    add_to_user_stack(thread, auxv, sizeof(auxv), sizeof(user_addr_t),
                      &stack_ptr);

    /* envp - for layout compatibility, unused */
    user_addr_t envp[] = {
            0,
    };
    add_to_user_stack(thread, envp, sizeof(envp), sizeof(user_addr_t),
                      &stack_ptr);

    /* argv. Only argv [0] and argv [1] (terminator) are set. */
    user_addr_t argv[] = {
            app_name_addr,
            0,
    };
    add_to_user_stack(thread, argv, sizeof(argv), sizeof(user_addr_t),
                      &stack_ptr);

    /* argc. The null terminator is not counted. */
    user_addr_t argc = countof(argv) - 1;
    user_addr_t argc_ptr = add_to_user_stack(thread, &argc, sizeof(argc),
                                             sizeof(user_addr_t), &stack_ptr);

    thread->start_stack = stack_ptr;

    return argc_ptr;
}

/**
 * select_load_bias() - Pick a a load bias for an ELF
 * @phdr:      Pre-validated program header array base
 * @num_phdrs: Number of program headers
 * @aspace:    The address space the bias needs to be valid in
 * @out:       Out pointer to write the selected bias to. Only valid if the
 *             function returned 0.
 *
 * This function calculates an offset that can be added to every loadable ELF
 * segment in the image and still result in a legal load address.
 *
 * Return: A status code indicating whether a bias was located. If nonzero,
 *         the bias output may be invalid.
 */
static status_t select_load_bias(ELF_PHDR* phdr,
                                 size_t num_phdrs,
                                 vmm_aspace_t* aspace,
                                 vaddr_t* out) {
    DEBUG_ASSERT(out);
#if ASLR
    vaddr_t low = VADDR_MAX;
    vaddr_t high = 0;
    for (size_t i = 0; i < num_phdrs; i++, phdr++) {
        low = MIN(low, phdr->p_vaddr);
        vaddr_t candidate_high;
        if (!__builtin_add_overflow(phdr->p_vaddr, phdr->p_memsz,
                                    &candidate_high)) {
            high = MAX(high, candidate_high);
        } else {
            dprintf(CRITICAL, "Segment %zu overflows virtual address space\n",
                    i);
            return ERR_NOT_VALID;
        }
    }
    LTRACEF("ELF Segment range: %" PRIxVADDR "->%" PRIxVADDR "\n", low, high);

    DEBUG_ASSERT(high >= low);
    size_t size = round_up(high - low, PAGE_SIZE);
    LTRACEF("Spot size: %zu\n", size);

    vaddr_t spot;
    if (!vmm_find_spot(aspace, size, &spot)) {
        return ERR_NO_MEMORY;
    }
    LTRACEF("Load target: %" PRIxVADDR "\n", spot);

    /*
     * Overflow is acceptable here, since adding the delta to the lowest
     * ELF load address will still return to spot, which was the goal.
     */
    __builtin_sub_overflow(spot, low, out);
#else
    /* If ASLR is disabled, the app is not PIE, use a load bias of 0 */
    *out = 0;
#endif

    dprintf(INFO, "Load bias: %lx\n", *out);

    return NO_ERROR;
}

static void finalize_registration(void)
{
	mutex_acquire(&apps_lock);
	apps_started = true;
	mutex_release(&apps_lock);
}

status_t register_app_notifier(app_notifier_t *n)
{
	status_t ret = NO_ERROR;

	mutex_acquire(&apps_lock);
	if (!apps_started)
		list_add_tail(&app_notifier_list, &n->node);
	else
		ret = ERR_ALREADY_STARTED;
	mutex_release(&apps_lock);
	return ret;
}

int als_alloc_slot(void)
{
	int ret;

	mutex_acquire(&apps_lock);
	if (!apps_started)
		ret = ++als_slot_cnt;
	else
		ret = ERR_ALREADY_STARTED;
	mutex_release(&apps_lock);
	return ret;
}


static void load_app_config_options(intptr_t app_image_addr,
		app_t *app, ELF_SHDR *shdr)
{
	char  *manifest_data;
	u_int *config_blob, config_blob_size;
	u_int i;
	u_int app_idx;

	/* have to at least have a valid UUID */
	ASSERT(shdr->sh_size >= sizeof(uuid_t));

	/* init default config options before parsing manifest */
	app->props.min_heap_size = 4 * PAGE_SIZE;
	app->props.min_stack_size = DEFAULT_STACK_SIZE;

	app_idx = app - app_list;

	manifest_data = (char *)(app_image_addr + shdr->sh_offset);

	memcpy(&app->props.uuid,
	       (uuid_t *)manifest_data,
	       sizeof(uuid_t));

	PRINT_APP_UUID(app_idx, &app->props.uuid);

	manifest_data += sizeof(app->props.uuid);

	memcpy(app->app_name, manifest_data, 256);

	manifest_data += 256;

	config_blob = (u_int *)manifest_data;
	config_blob_size = (shdr->sh_size - sizeof(uuid_t) - 256);

	app->props.config_entry_cnt = config_blob_size / sizeof (u_int);

	/* if no config options we're done */
	if (app->props.config_entry_cnt == 0) {
		return;
	}

	/* save off configuration blob start so it can be accessed later */
	app->props.config_blob = config_blob;

	/*
	 * Step thru configuration blob.
	 *
	 * Save off some configuration data while we are here but
	 * defer processing of other data until it is needed later.
	 */
	for (i = 0; i < app->props.config_entry_cnt; i++) {
		switch (config_blob[i]) {
		case APP_CONFIG_KEY_MIN_STACK_SIZE:
			/* MIN_STACK_SIZE takes 1 data value */
			ASSERT((app->props.config_entry_cnt - i) > 1);
			app->props.min_stack_size =
				ROUNDUP(config_blob[++i], 4096);
			ASSERT(app->props.min_stack_size > 0);
			break;
		case APP_CONFIG_KEY_MIN_HEAP_SIZE:
			/* MIN_HEAP_SIZE takes 1 data value */
			ASSERT((app->props.config_entry_cnt - i) > 1);
			app->props.min_heap_size =
				ROUNDUP(config_blob[++i], 4096);
			ASSERT(app->props.min_heap_size > 0);
			break;
		case APP_CONFIG_KEY_MAP_MEM:
			/* MAP_MEM takes 3 data values */
			ASSERT((app->props.config_entry_cnt - i) > 3);
			app->props.map_io_mem_cnt++;
			i += 3;
			break;
		default:
			dprintf(CRITICAL,
				"%s: unknown config key: %d\n",
				__func__, config_blob[i]);
			ASSERT(0);
			i++;
			break;
		}
	}

#if DEBUG_LOAD_APP
	dprintf(SPEW, "app %p: stack_sz=0x%x\n", app,
		app->props.min_stack_size);
	dprintf(SPEW, "app %p: heap_sz=0x%x\n", app,
		app->props.min_heap_size);
	dprintf(SPEW, "app %p: num_io_mem=%d\n", app,
		app->props.map_io_mem_cnt);
#endif
}

static status_t init_brk(app_t *app)
{
	status_t status;
	vaddr_t vaddr;
	u_int flags = ARCH_MMU_FLAG_CACHED | ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_NO_EXECUTE | ARCH_MMU_FLAG_NS;

	app->cur_brk = app->start_brk;

	/* do we need to increase user mode heap (if not enough remains)? */
	if ((app->end_brk - app->start_brk) >=
	    app->props.min_heap_size)
		return NO_ERROR;

	vaddr = app->end_brk;

	status = vmm_alloc_contiguous(app->ut->thread->aspace, "brk", app->props.min_heap_size, &vaddr, 0, 
				VMM_FLAG_VALLOC_SPECIFIC, flags);
			     
	if (status != NO_ERROR || vaddr != app->end_brk) {
		dprintf(CRITICAL, "cannot map brk\n");
		return ERR_NO_MEMORY;
	}

	app->end_brk += app->props.min_heap_size;
	return NO_ERROR;
}

static status_t alloc_address_map(app_t *app)
{
	ELF_EHDR *elf_hdr = app->app_img;
	void *app_image;
	ELF_PHDR *prg_hdr;
	u_int i, app_idx;
	status_t ret;
	vaddr_t start_code = ~0;
	vaddr_t start_data = 0;
	vaddr_t end_code = 0;
	vaddr_t end_data = 0;

	app_image = app->app_img;
	app_idx = app - app_list;

	prg_hdr = (ELF_PHDR *)(app_image + elf_hdr->e_phoff);

	status_t bias_result =
		select_load_bias(prg_hdr, elf_hdr->e_phnum, app->ut->thread->aspace,
				&app->load_bias);
	if (bias_result) {
		return bias_result;
	}

	/* create mappings for PT_LOAD sections */
	for (i = 0; i < elf_hdr->e_phnum; i++) {
		vaddr_t first, last, last_mem;

		prg_hdr = (ELF_PHDR *)(app_image + elf_hdr->e_phoff +
				(i * sizeof(ELF_PHDR)));

#if DEBUG_LOAD_APP
		dprintf(SPEW,
			"app %d: ELF type 0x%x, vaddr 0x%08x, paddr 0x%08x"
			" rsize 0x%08x, msize 0x%08x, flags 0x%08x\n",
			app_idx, prg_hdr->p_type, prg_hdr->p_vaddr,
			prg_hdr->p_paddr, prg_hdr->p_filesz, prg_hdr->p_memsz,
			prg_hdr->p_flags);
#endif

		if (prg_hdr->p_type != PT_LOAD)
			continue;

		/* skip PT_LOAD if it's below app start or above .bss */
		if ((prg_hdr->p_vaddr < APP_START_ADDR) ||
		    (prg_hdr->p_vaddr >= app->end_bss))
			continue;

		/*
		 * We're expecting to be able to execute the app in-place,
		 * meaning its PT_LOAD segments, should be page-aligned.
		 */
		ASSERT(!(prg_hdr->p_vaddr & PAGE_MASK) &&
		       !(prg_hdr->p_offset & PAGE_MASK));

		size_t size = (prg_hdr->p_memsz + PAGE_MASK) & ~PAGE_MASK;
		paddr_t paddr = vaddr_to_paddr(app_image + prg_hdr->p_offset);
		vaddr_t vaddr = prg_hdr->p_vaddr;
		u_int flags = PF_TO_UTM_FLAGS(prg_hdr->p_flags) | ARCH_MMU_FLAG_CACHED | ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_NS;

		ret = vmm_alloc_physical(app->ut->thread->aspace, "c/d", size, &vaddr, 0, paddr, VMM_FLAG_VALLOC_SPECIFIC, flags);

		if (ret) {
			dprintf(CRITICAL, "cannot map the segment\n");
			return ret;
		}

		vaddr_t stack_bot = APP_STACK_TOP - app->props.min_stack_size;
		/* check for overlap into user stack range */
		if (stack_bot < vaddr + size) {
			dprintf(CRITICAL,
				"failed to load app: (overlaps user stack 0x%lx)\n",
				 stack_bot);
			return ERR_TOO_BIG;
		}

#if DEBUG_LOAD_APP
		dprintf(SPEW,
			"app %d: load vaddr 0x%08lx, paddr 0x%08lx,"
			" rsize 0x%08lx, msize 0x%08x, access %c%c%c,"
			" flags 0x%x\n",
			app_idx, vaddr, paddr, size, prg_hdr->p_memsz,
			prg_hdr->p_flags & PF_R ? 'r' : '-', prg_hdr->p_flags & PF_W ? 'w' : '-',
			prg_hdr->p_flags & PF_X ? 'x' : '-', prg_hdr->p_flags);
#endif

		/* start of code/data */
		first = prg_hdr->p_vaddr;
		if (first < start_code)
			start_code = first;
		if (start_data < first)
			start_data = first;

		/* end of code/data */
		last = prg_hdr->p_vaddr + prg_hdr->p_filesz;
		if ((prg_hdr->p_flags & PF_X) && end_code < last)
			end_code = last;
		if (end_data < last)
			end_data = last;

		/* end of brk */
		last_mem = prg_hdr->p_vaddr + prg_hdr->p_memsz;
		if (last_mem > app->start_brk) {
			void *segment_start = app_image + prg_hdr->p_offset;

			app->start_brk = last_mem;
			/* make brk consume the rest of the page */
			app->end_brk = prg_hdr->p_vaddr + size;

			/* zero fill the remainder of the page for brk.
			 * do it here (instead of init_brk) so we don't
			 * have to keep track of the kernel address of
			 * the mapping where brk starts */
			memset(segment_start + prg_hdr->p_memsz, 0,
			       size - prg_hdr->p_memsz);
		}
	}

	ret = init_brk(app);
	if (ret != NO_ERROR) {
		dprintf(CRITICAL, "failed to load app: app heap creation error\n");
		return ret;
	}

	dprintf(SPEW, "app %d: code: start 0x%08lx end 0x%08lx\n",
		app_idx, start_code, end_code);
	dprintf(SPEW, "app %d: data: start 0x%08lx end 0x%08lx\n",
		app_idx, start_data, end_data);
	dprintf(SPEW, "app %d: bss:                end 0x%08lx\n",
		app_idx, app->end_bss);
	dprintf(SPEW, "app %d: brk:  start 0x%08lx end 0x%08lx\n",
		app_idx, app->start_brk, app->end_brk);

	dprintf(SPEW, "app %d: entry 0x%08lx\n", app_idx, app->ut->entry);

	return NO_ERROR;
}

/*
 * Align the next app to a page boundary, by copying what remains
 * in the app image to the aligned next app start. This should be
 * called after we're done with the section headers as the previous
 * apps .shstrtab section will be clobbered.
 *
 * Note: app_image_size remains the carved out part in OLDK to exit
 * the bootloader loop, so still increment by max_extent. Because of
 * the copy down to an aligned next app addr, app_image_size is
 * more than what we're actually using.
 */
static char *align_next_app(ELF_EHDR *elf_hdr, ELF_SHDR *pad_hdr,
			    u_int max_extent)
{
	char *next_app_align_start;
	char *next_app_fsize_start;
	char *app_image_addr;
	u_int copy_size;

	ASSERT(ROUNDUP(max_extent, 4) == elf_hdr->e_shoff);
	ASSERT(pad_hdr);

	app_image_addr = (char *)elf_hdr;
	max_extent = (elf_hdr->e_shoff + (elf_hdr->e_shnum * elf_hdr->e_shentsize)) - 1;
	ASSERT((app_image_addr + max_extent + 1) <= app_image_end);

	next_app_align_start = app_image_addr + pad_hdr->sh_offset + pad_hdr->sh_size;
	next_app_fsize_start = app_image_addr + max_extent + 1;
	ASSERT(next_app_align_start <= next_app_fsize_start);

	copy_size = app_image_end - next_app_fsize_start;
	if (copy_size) {
		/*
		 * Copy remaining image bytes to aligned start for the next
		 * (and subsequent) apps. Also decrement app_image_end, so
		 * we copy less each time we realign for the next app.
		 */
		memcpy(next_app_align_start, next_app_fsize_start, copy_size);
		arch_sync_cache_range((addr_t)next_app_align_start,
				       copy_size);
		app_image_end -= (next_app_fsize_start - next_app_align_start);
	}

	app_image_size -= (max_extent + 1);
	return next_app_align_start;
}

/*
 * Look in the kernel's ELF header for app sections and
 * carveout memory for their LOAD-able sections.
 */
static void app_bootloader(void)
{
	ELF_EHDR *ehdr;
	ELF_SHDR *shdr;
	ELF_SHDR *bss_shdr, *bss_pad_shdr, *manifest_shdr;
	char *shstbl, *app_image_addr;
	app_t *app = 0;

	dprintf(SPEW, "app: start %p size 0x%08x end %p\n",
		app_image_start, app_image_size, app_image_end);

	app_image_addr = app_image_start;

	/* alloc app_list page from carveout */
	if (app_image_size) {
		app_list = (app_t *)memalign(PAGE_SIZE, PAGE_SIZE);
		app = app_list;
		memset(app, 0, PAGE_SIZE);
	}

	while (app_image_size > 0) {
		u_int i, app_max_extent;

		ehdr = (ELF_EHDR *) app_image_addr;
		if (strncmp((char *)ehdr->e_ident, ELFMAG, SELFMAG)) {
			dprintf(CRITICAL, "app_bootloader: ELF header not found\n");
			break;
		}

		shdr = (ELF_SHDR *) ((intptr_t)ehdr + ehdr->e_shoff);
		shstbl = (char *)((intptr_t)ehdr + shdr[ehdr->e_shstrndx].sh_offset);

		app_max_extent = 0;
		bss_shdr = bss_pad_shdr = manifest_shdr = NULL;

		/* calculate app end */
		for (i = 0; i < ehdr->e_shnum; i++) {
			u_int extent;

			if (shdr[i].sh_type == SHT_NULL)
				continue;
#if DEBUG_LOAD_APP
			dprintf(SPEW, "app: sect %d, off 0x%08x, size 0x%08x, flags 0x%02x, name %s\n",
				i, shdr[i].sh_offset, shdr[i].sh_size, shdr[i].sh_flags, shstbl + shdr[i].sh_name);
#endif

			/* track bss and manifest sections */
			if (!strcmp((shstbl + shdr[i].sh_name), ".bss")) {
				bss_shdr = shdr + i;
				app->end_bss = bss_shdr->sh_addr + bss_shdr->sh_size;
			}
			else if (!strcmp((shstbl + shdr[i].sh_name), ".bss-pad")) {
				bss_pad_shdr = shdr + i;
			}
			else if (!strcmp((shstbl + shdr[i].sh_name),
					 ".app.manifest")) {
				manifest_shdr = shdr + i;
			}

			if (shdr[i].sh_type != SHT_NOBITS) {
				extent = shdr[i].sh_offset + shdr[i].sh_size;
				if (app_max_extent < extent)
					app_max_extent = extent;
			}
		}

		/* we need these sections */
		ASSERT(bss_shdr && bss_pad_shdr && manifest_shdr);

		/* clear .bss */
		ASSERT((bss_shdr->sh_offset + bss_shdr->sh_size) <= app_max_extent);
		memset((uint8_t *)app_image_addr + bss_shdr->sh_offset, 0, bss_shdr->sh_size);

		load_app_config_options((intptr_t)app_image_addr, app, manifest_shdr);
		app->app_img = ehdr;

		/* align next app start */
		app_image_addr = align_next_app(ehdr, bss_pad_shdr, app_max_extent);

		ASSERT((app_count + 1) < MAX_APP_COUNT);
		app_count++;
		app++;
	}
}

status_t app_setup_mmio(app_t *app, u_int mmio_id,
		vaddr_t *vaddr, uint32_t map_size)
{
	u_int i;
	u_int id, offset, size;

	/* step thru configuration blob looking for I/O mapping requests */
	for (i = 0; i < app->props.config_entry_cnt; i++) {
		if (app->props.config_blob[i] == APP_CONFIG_KEY_MAP_MEM) {
			u_int flags = ARCH_MMU_FLAG_UNCACHED_DEVICE | ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_NO_EXECUTE |
				      ARCH_MMU_FLAG_NS;
	
			id = app->props.config_blob[++i];
			offset = app->props.config_blob[++i];
			size = ROUNDUP(app->props.config_blob[++i],
					PAGE_SIZE);

			if (id != mmio_id)
				continue;

			map_size = ROUNDUP(map_size, PAGE_SIZE);
			if (map_size > size)
				return ERR_INVALID_ARGS;

			return vmm_alloc_physical(app->ut->thread->aspace, "io", map_size, &vaddr, 0, offset,
				VMM_FLAG_VALLOC_SPECIFIC, flags);			
		} else {
			/* all other config options take 1 data value */
			i++;
		}
	}

	return ERR_NOT_FOUND;
}

void app_init(void)
{
	app_t *app;
	u_int i;

	app_image_start = (char *)&__app_start;
	app_image_end = (char *)&__app_end;
	app_image_size = (app_image_end - app_image_start);

	ASSERT(!((uintptr_t)app_image_start & PAGE_MASK));

	finalize_registration();

	app_bootloader();

	for (i = 0, app = app_list; i < app_count; i++, app++) {
		char name[32];
		uthread_t *uthread;
		int ret;

		snprintf(name, sizeof(name), "%s, app_%d_%08x-%04x-%04x",
			 app->app_name,
			 i,
			 app->props.uuid.time_low,
			 app->props.uuid.time_mid,
			 app->props.uuid.time_hi_and_version);

		/* entry is 0 at this point since we haven't parsed the elf hdrs
		 * yet */
		ELF_EHDR *elf_hdr = app->app_img;
		uthread = uthread_create(name, elf_hdr->e_entry,
					 DEFAULT_PRIORITY, APP_STACK_TOP,
					 app->props.min_stack_size, app);
		if (uthread == NULL) {
			/* TODO: do better than this */
			panic("allocate user thread failed\n");
		}
		app->ut = uthread;
		uthread->app = app;

		ret = alloc_address_map(app);
		if (ret != NO_ERROR) {
			panic("failed to load address map\n");
		}

		/* attach als_cnt */
		app->als = calloc(1, als_slot_cnt * sizeof(void*));
		if (!app->als) {
			panic("allocate app local storage failed\n");
		}

		/* call all registered startup notifiers */
		app_notifier_t *n;
		list_for_every_entry(&app_notifier_list, n, app_notifier_t, node) {
			if (n->startup) {
				ret = n->startup(app);
				if (ret != NO_ERROR)
					panic("failed (%d) to invoke startup notifier\n", ret);
			}
		}
	}
}

app_t *app_find_by_uuid(uuid_t *uuid)
{
	app_t *ta;
	u_int i;

	/* find app for this uuid */
	for (i = 0, ta = app_list; i < app_count; i++, ta++)
		if (!memcmp(&ta->props.uuid, uuid, sizeof(uuid_t)))
			return ta;

	return NULL;
}

/* rather export app_list?  */
void app_forall(void (*fn)(app_t *ta, void *data), void *data)
{
	u_int i;
	app_t *ta;

	if (fn == NULL)
		return;

	for (i = 0, ta = app_list; i < app_count; i++, ta++)
		fn(ta, data);
}

static void start_apps(uint level)
{
	app_t *app;
	u_int i;
	int ret;

	for (i = 0, app = app_list; i < app_count; i++, app++) {
		if (app->ut->entry) {
			ret = uthread_start(app->ut);
			if (ret)
				panic("Cannot start Trusty app!\n");
		}
	}
}

INIT_HOOK(libapps, start_apps, INIT_LEVEL_APPS + 1);
