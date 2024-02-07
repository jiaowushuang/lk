#ifndef _HELIOS_BITMAP_H_
#define _HELIOS_BITMAP_H_

#include <sys/types.h>
#include <kern/bitops.h>

#ifndef __ASSEMBLER__

#define BITMAP_ARRAY_SIZE(num, align) (((num) + align - 1ul) / align)
#define BITMAP_ALIGN (sizeof(word_t) * 8)
#define BITMAP_INDEX(bits) (bits / BITMAP_ALIGN)
#define BITMAP_OFFSET(bits) (bits % BITMAP_ALIGN)
#define BITMAP_MASK(bits) BIT(BITMAP_OFFSET(bits))
#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))
#define BITMAP_LAST_WORD_MASK(nbits) (~0UL >> (-(nbits) & (BITS_PER_LONG - 1)))
#define BITMAP_SIZE(size) (BITS_TO_LONGS((size)) * sizeof(long))
#define DECLARE_BITMAP(name, bits) unsigned long name[BITS_TO_LONGS(bits)]

typedef uint32_t bitmap_granule_t;
typedef bitmap_granule_t *bitmap_t;
static const bitmap_granule_t ONE = 1;

#define BITMAP_GRANULE_LEN (sizeof(bitmap_granule_t) * 8)

#define BITMAP_ALLOC(NAME, SIZE)                                               \
	bitmap_granule_t NAME[(SIZE / BITMAP_GRANULE_LEN) +                    \
			      (SIZE % BITMAP_GRANULE_LEN ? 1 : 0)]

#define BITMAP_ALLOC_ARRAY(NAME, SIZE, NUM)                                    \
	bitmap_granule_t NAME[NUM][(SIZE / BITMAP_GRANULE_LEN) +               \
				   (SIZE % BITMAP_GRANULE_LEN ? 1 : 0)]

static inline void bitmap_set(bitmap_t map, size_t bit)
{
	map[bit / BITMAP_GRANULE_LEN] |= ONE << (bit % BITMAP_GRANULE_LEN);
}

static inline void bitmap_clear(bitmap_t map, size_t bit)
{
	map[bit / BITMAP_GRANULE_LEN] &= ~(ONE << (bit % BITMAP_GRANULE_LEN));
}

static inline uint64_t bitmap_get(bitmap_t map, size_t bit)
{
	return (map[bit / BITMAP_GRANULE_LEN] &
		(ONE << (bit % BITMAP_GRANULE_LEN))) ?
		       1U :
		       0U;
}
#endif /* !__ASSEMBLER__ */

#endif
