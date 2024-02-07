#pragma once

#include <sys/types.h>
#include <kern/utils.h>

/* bit manipulation */
#define BM(base, count, val) (((val) & ((UL(1) << (count)) - 1)) << (base))
#define AM(base, count) (((UL(1) << (count)) - 1) << (base))

#define SU(me, val) ((me) |= (val))
#define CU(me, val) ((me) &= (~(val)))
#define GU(me, mask) ((me) & (mask))
#define MU(me, mask, val) ((me) = ((me) & (~GU(me, mask))) | (val))

#define GUB(me, mask, base) (((me) & (mask)) >> (base))
#define GUI(me, mask, base, val) (GUB(me, mask, base) + (val))
#define GUD(me, mask, base, val) (GUB(me, mask, base) - (val))

#define SR(me, base, count, val) (SU(me, BM(base, count, val)))
#define GR(me, base, count) (GU(me, AM(base, count)) >> (base))
#define CR(me, base, count, val) (CU(me, BM(base, count, val)))
#define ZR(me, base, count) (CU(me, AM(base, count)))
#define MR(me, base, count, val) (MU(me, AM(base, count), BM(base, count, val)))

#define IR(me, base, count, val)                                               \
	(MU(me, AM(base, count),                                               \
	    BM(base, count, GUI(me, AM(base, count), base, val))))
#define DR(me, base, count, val)                                               \
	(MU(me, AM(base, count),                                               \
	    BM(base, count, GUD(me, AM(base, count), base, val))))

#define SB(me, base) ((me) |= BIT(base))
#define BITOPS_GB(me, base) (((me)&BIT(base)) >> (base))
#define CB(me, base) ((me) &= ~BIT(base))

#define BITS_PER_BYTE (8)
#define BITS_PER_LONG 64
#define BITS_PER_LONG_LONG 64

#define BIT(nr) (UL(1) << (nr))
#define BITULL(nr) (ULL(1) << (nr))
#define MASK(n) (BIT(n) - UL(1))
#define BIT_MASK(nr) (UL(1) << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr) ((nr) / BITS_PER_LONG)
#define BITS_PER_TYPE(type) (sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr) DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define clzl(x) __builtin_clzl(x)
#define ctzl(x) __builtin_ctzl(x)


#ifndef __ASSEMBLER__
/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static inline unsigned long __ffs(unsigned long word)
{
	int num = 0;

#if BITS_PER_LONG == 64
	if ((word & 0xffffffff) == 0) {
		num += 32;
		word >>= 32;
	}
#endif
	if ((word & 0xffff) == 0) {
		num += 16;
		word >>= 16;
	}
	if ((word & 0xff) == 0) {
		num += 8;
		word >>= 8;
	}
	if ((word & 0xf) == 0) {
		num += 4;
		word >>= 4;
	}
	if ((word & 0x3) == 0) {
		num += 2;
		word >>= 2;
	}
	if ((word & 0x1) == 0)
		num += 1;
	return num;
}

static inline unsigned long __fls(unsigned long word)
{
	int num = BITS_PER_LONG - 1;

#if BITS_PER_LONG == 64
	if (!(word & (~0ul << 32))) {
		num -= 32;
		word <<= 32;
	}
#endif
	if (!(word & (~0ul << (BITS_PER_LONG - 16)))) {
		num -= 16;
		word <<= 16;
	}
	if (!(word & (~0ul << (BITS_PER_LONG - 8)))) {
		num -= 8;
		word <<= 8;
	}
	if (!(word & (~0ul << (BITS_PER_LONG - 4)))) {
		num -= 4;
		word <<= 4;
	}
	if (!(word & (~0ul << (BITS_PER_LONG - 2)))) {
		num -= 2;
		word <<= 2;
	}
	if (!(word & (~0ul << (BITS_PER_LONG - 1))))
		num -= 1;
	return num;
}

static inline long fls(unsigned long x)
{
	return x ? sizeof(x) * 8 - __builtin_clzl(x) : 0;
}

#if BITS_PER_LONG == 32
static inline int fls64(unsigned long x)
{
	__u32 h = x >> 32;

	if (h)
		return fls(h) + 32;
	return fls(x);
}
#elif BITS_PER_LONG == 64
static inline int fls64(unsigned long x)
{
	if (x == 0)
		return 0;
	return __fls(x) + 1;
}
#else
#error BITS_PER_LONG not 32 or 64
#endif

#define ffz(x) __ffs(~(x))

/*
 * Include this here because some architectures need generic_ffs/fls64 in
 * scope
 */
void set_bit(int nr, unsigned long *p);
void clear_bit(int nr, unsigned long *p);
int test_and_set_bit(int nr, unsigned long *p);
int test_and_clear_bit(int nr, unsigned long *p);
int test_and_change_bit(int nr, unsigned long *p);
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
			    unsigned long offset);
unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size,
				 unsigned long offset);

unsigned long find_first_bit(const unsigned long *addr, unsigned long size);
unsigned long find_first_zero_bit(const unsigned long *addr,
				  unsigned long size);
unsigned long find_last_bit(const unsigned long *addr, unsigned long size);

#define for_each_set_bit(bit, addr, size)                                      \
	for ((bit) = find_first_bit((addr), (size)); (bit) < (size);           \
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

/* same as for_each_set_bit() but use bit as value to start with */
#define for_each_set_bit_from(bit, addr, size)                                 \
	for ((bit) = find_next_bit((addr), (size), (bit)); (bit) < (size);     \
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

#define for_each_clear_bit(bit, addr, size)                                    \
	for ((bit) = find_first_zero_bit((addr), (size)); (bit) < (size);      \
	     (bit) = find_next_zero_bit((addr), (size), (bit) + 1))

/* same as for_each_clear_bit() but use bit as value to start with */
#define for_each_clear_bit_from(bit, addr, size)                               \
	for ((bit) = find_next_zero_bit((addr), (size), (bit));                \
	     (bit) < (size);                                                   \
	     (bit) = find_next_zero_bit((addr), (size), (bit) + 1))

#define BIT_MASK_OFF(OFF, LEN) ((((1ULL << ((LEN)-1)) << 1) - 1) << (OFF))

uint64_t bit_ctz(uint64_t n);
uint64_t bit_clz(uint64_t n);

static inline uint64_t bit_get(uint64_t word, uint64_t off)
{
	return word & (1UL << off);
}

static inline uint64_t bit_set(uint64_t word, uint64_t off)
{
	return word |= (1UL << off);
}

static inline uint64_t bit_clear(uint64_t word, uint64_t off)
{
	return word &= ~(1UL << off);
}

static inline uint64_t bit_extract(uint64_t word, uint64_t off, uint64_t len)
{
	return (word >> off) & BIT_MASK_OFF(0, len);
}

static inline uint64_t bit_insert(uint64_t word, uint64_t val, uint64_t off,
				  uint64_t len)
{
	return (~BIT_MASK_OFF(off, len) & word) |
	       ((BIT_MASK_OFF(0, len) & val) << off);
}
#endif /* !__ASSEMBLER__ */


