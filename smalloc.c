#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "mutex.h"
#include "arch/arch.h"
#include "smalloc.h"

//#define SMALLOC_REDZONE		/* define to detect memory corruption */

#define SMALLOC_BPB	32	/* block size, bytes-per-bit in bitmap */
#define SMALLOC_BPI	(sizeof(unsigned int) * 8)
#define SMALLOC_BPL	(SMALLOC_BPB * SMALLOC_BPI)

#define INITIAL_SIZE	16*1024*1024	/* new pool size */
#define INITIAL_POOLS	8		/* maximum number of pools to setup */

#define MAX_POOLS	16

unsigned int smalloc_pool_size = INITIAL_SIZE;

struct pool {
	struct fio_mutex *lock;			/* protects this pool */
	void *map;				/* map of blocks */
	unsigned int *bitmap;			/* blocks free/busy map */
	size_t free_blocks; 			/* free blocks */
	size_t nr_blocks;			/* total bitmap blocks */
	size_t next_non_full;
	size_t mmap_size;
};

struct block_hdr {
	size_t size;
#ifdef SMALLOC_REDZONE
	unsigned int prered;
#endif
};

static struct pool mp[MAX_POOLS];
static unsigned int nr_pools;
static unsigned int last_pool;

static inline size_t size_to_blocks(size_t size)
{
	return (size + SMALLOC_BPB - 1) / SMALLOC_BPB;
}

static int blocks_iter(struct pool *pool, unsigned int pool_idx,
		       unsigned int idx, size_t nr_blocks,
		       int (*func)(unsigned int *map, unsigned int mask))
{

	while (nr_blocks) {
		unsigned int this_blocks, mask;
		unsigned int *map;

		if (pool_idx >= pool->nr_blocks)
			return 0;

		map = &pool->bitmap[pool_idx];

		this_blocks = nr_blocks;
		if (this_blocks + idx > SMALLOC_BPI) {
			this_blocks = SMALLOC_BPI - idx;
			idx = SMALLOC_BPI - this_blocks;
		}

		if (this_blocks == SMALLOC_BPI)
			mask = -1U;
		else
			mask = ((1U << this_blocks) - 1) << idx;

		if (!func(map, mask))
			return 0;

		nr_blocks -= this_blocks;
		idx = 0;
		pool_idx++;
	}

	return 1;
}

static int mask_cmp(unsigned int *map, unsigned int mask)
{
	return !(*map & mask);
}

static int mask_clear(unsigned int *map, unsigned int mask)
{
	assert((*map & mask) == mask);
	*map &= ~mask;
	return 1;
}

static int mask_set(unsigned int *map, unsigned int mask)
{
	assert(!(*map & mask));
	*map |= mask;
	return 1;
}

/* judge if the pool has the nr_blocks blocks */
static int blocks_free(struct pool *pool, unsigned int pool_idx,
		       unsigned int idx, size_t nr_blocks)
{
	return blocks_iter(pool, pool_idx, idx, nr_blocks, mask_cmp);
}

/* set the bitmap in the pool for nr_blocks blocks */
static void set_blocks(struct pool *pool, unsigned int pool_idx,
		       unsigned int idx, size_t nr_blocks)
{
	blocks_iter(pool, pool_idx, idx, nr_blocks, mask_set);
}

/* clear the bitmap in the pool for nr_blocks blocks */
static void clear_blocks(struct pool *pool, unsigned int pool_idx,
			 unsigned int idx, size_t nr_blocks)
{
	blocks_iter(pool, pool_idx, idx, nr_blocks, mask_clear);
}

/* find first zero bit in the word from index start 
 * e.g 
 * 	word = 0x1D(00011101b),  start = 3
 * 	
 * 	00011101b
 * 	    ^
 * 	    |(start:3)
 *
 * return value
 * 	5
 */
int find_next_zero(int word, int start)
{
	word >>= start;	
	return ffz(word) + start;
}

static bool add_pool(struct pool *pool, unsigned int alloc_size)
{
	int bitmap_blocks;
	int mmap_flags;
	void *ptr;

	if (nr_pools == MAX_POOLS)
		return false;

#ifdef SMALLOC_REDZONE
	alloc_size += sizeof(unsigned int);
#endif
	alloc_size += sizeof(struct block_hdr);
	if (alloc_size < INITIAL_SIZE)
		alloc_size = INITIAL_SIZE;

	/* round up to nearest full number of blocks */
	alloc_size = (alloc_size + SMALLOC_BPL - 1) & ~(SMALLOC_BPL - 1);
	bitmap_blocks = alloc_size / SMALLOC_BPL;
	alloc_size += bitmap_blocks * sizeof(unsigned int);
	pool->mmap_size = alloc_size;

	pool->nr_blocks = bitmap_blocks;
	pool->free_blocks = bitmap_blocks * SMALLOC_BPB;

	mmap_flags = MAP_ANON;
#ifdef CONFIG_ESX
	mmap_flags |= MAP_PRIVATE;
#else
	mmap_flags |= MAP_SHARED;
#endif
	ptr = mmap(NULL, alloc_size, PROT_READ|PROT_WRITE, mmap_flags, -1, 0);

	if (ptr == MAP_FAILED)
		goto out_fail;

	pool->map = ptr;
	pool->bitmap = (unsigned int *)((char *) ptr + (pool->nr_blocks * SMALLOC_BPL));
	memset(pool->bitmap, 0, bitmap_blocks * sizeof(unsigned int));

#if 0
	pool->lock = fio_mutex_init(FIO_MUTEX_UNLOCKED);
	if (!pool->lock)
		goto out_fail;
#endif

	nr_pools++;
	return true;
out_fail:
	//log_err("smalloc: failed adding pool\n");
	if (pool->map)
		munmap(pool->map, pool->mmap_size);
	return false;
}

void sinit(void)
{
	bool ret;
	int i;

	for (i = 0; i < INITIAL_POOLS; i++) {
		ret = add_pool(&mp[nr_pools], smalloc_pool_size);
		if (!ret)
			break;
	}

	/* at least one pool should be added */
	assert(i);
}

#ifdef SMALLOC_REDZONE

#else
static void fill_redzone(struct block_hdr *hdr)
{

}
#endif

static void *__smalloc_pool(struct pool *pool, size_t size)
{
	void *ptr = NULL;
	size_t nr_blocks;
	int i; 			// bitmap block index
	int idx; 		// block index in one bitmap block.  0<=idx<SMALLOC_BPI
	int offset;
	int last_idx;

	// step 1 -- get the block count from params size
	nr_blocks = size_to_blocks(size);
	if (nr_blocks > pool->free_blocks)
		goto fail;

	// step 2 -- find enough blocks
	i = pool->next_non_full;
	last_idx = 0;
	while (i < pool->nr_blocks) {

		// find next non full bitmap block
		if (pool->bitmap[i] == -1U) {
			i++;
			pool->next_non_full = i;
			last_idx = 0;
			continue;
		}

		idx = find_next_zero(pool->bitmap[i], last_idx);
		if (!blocks_free(pool, i, idx, nr_blocks)) {
			idx += nr_blocks;
			if (idx < SMALLOC_BPI) { // still in the same bitmap block
				last_idx = idx;
			} else { // beyond the bitmap block
				last_idx = 0;
				while (idx >= SMALLOC_BPI) {
					i++;
					idx -= SMALLOC_BPI;
				}
			}
			continue;
		} else {
			set_blocks(pool, i, idx, nr_blocks);
			offset = i * SMALLOC_BPL + idx * SMALLOC_BPI;
			break;
		}
	}

	if (i < pool->nr_blocks)
		ptr = pool->map + offset;
fail:
	return ptr;
}

static void *smalloc_pool(struct pool *pool, size_t size)
{
	void *ptr;
	size_t alloc_size = size + sizeof(struct block_hdr);

	ptr = __smalloc_pool(&mp[0], alloc_size);
	if (ptr != NULL) {
		struct block_hdr *hdr = ptr;

		hdr->size = alloc_size;
		fill_redzone(hdr);

		ptr += sizeof(*hdr);
		memset(ptr, 0x0, size);
	}

	return ptr;
}

void *smalloc(size_t size)
{
	void *ptr;

	ptr = smalloc_pool(&mp[0], size);

	return ptr;
}

