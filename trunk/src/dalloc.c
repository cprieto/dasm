/**
 * @file
 *
 * The allocator is controlled by the pre-processor macro _DASM_ARENA_ALLOC.
 * If _DASM_ARENA_ALLOC exists, the special arena-style allocator is used
 * instead of the basic malloc(3)/free(3) allocator. This tends to improve
 * memory allocation performance, however it also prevents valgrind(1) and
 * related tools from working well. Also, the arena-style allocator frees
 * memory only at the end, so memory requirements may be higher.
 *
 * @todo The arena-style allocator should be extended to handle arbitrarily
 * large allocations as well. If a request for more than ALLOCSIZE comes in,
 * just create a new block of the right size and slap it on the stack *under*
 * the current block we are satisfying small allocations out of. Sounds simple
 * enough? NOTE THAT THIS IS SOMEWHAT IMPORTANT SINCE THE SYMBOL TABLE SORTING
 * CODE COULD MAKE A LARGE ALLOCATION WHICH WOULD FAIL RIGHT NOW!
 *
 * @todo The whole #ifdef business leaves a bad taste in my mouth.
 */

/* if this is defined, we use the arena-style allocator */
#define _DASM_ARENA_ALLOC 1

#include "dalloc.h"

#include "platform.h"
#include "errors.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
 * [phf] Some debugging aids for measuring memory allocation patterns.
 * I am sure there's a tool that does this for regular malloc(3) stuff
 * but that wouldn't work for the small_alloc() business anyway. So I
 * hacked this little framework. I am not sure in what form it should
 * survive later. Ideas? (The implementation is at the bottom.)
 */

static void debug_record_arena_alloc(size_t bytes);
static void debug_record_regular_alloc(size_t bytes);

/**
 * Wrapper for malloc(3) that terminates with panic_fmt() if no
 * memory is available.
 *
 * @pre bytes > 0
 */
static void *checked_malloc(size_t bytes) __attribute__((malloc));
static void *checked_malloc(size_t bytes)
{
	debug_record_regular_alloc(bytes);
	assert(bytes > 0);

	void *p = malloc(bytes);
	if (p == NULL) {
		panic_fmt(PANIC_MEMORY, bytes, SOURCE_LOCATION);
	}

	return p;
}

/**
 * Wrapper for checked_malloc() that zeroes the allocated memory
 * using memset(3).
 *
 * @pre bytes > 0
 *
 * @warning Sadly memset(3) may not initialize pointers or floats
 * correctly (to NULL or 0.0) on some (very strange) machines.
 */
static void *zero_malloc(size_t bytes) __attribute__((malloc));
static void *zero_malloc(size_t bytes)
{
	assert(bytes > 0);

	void *p = checked_malloc(bytes);
	p = memset(p, 0, bytes);

	return p;
}

#ifdef _DASM_ARENA_ALLOC
struct new_perm_block
{
	struct new_perm_block *next;
	char data[];
};

static struct new_perm_block *new_permalloc_stack = NULL;

#define ALLOCSIZE 16384
#define ROUNDUP(x) ((x + alignment-1) & ~(alignment-1))

/**
 * Efficiently allocate small amounts of memory.
 *
 * @pre bytes > 0
 *
 * @warning As of right now, you can only request *small* amounts
 * of memory from this function, less than 1024 bytes at a time
 * is a good rule of thumb.
 * You *cannot* free(3) the pointer returned by small_alloc(),
 * truly bad things will happen if you try.
 * You can *only* free *all* the memory ever allocated through
 * small_alloc() using small_free_all() below.
 *
 * @todo Extend to arbitrary allocations as outlined above.
 * Also clean up formatting and refactor.
 */
static void *small_alloc(size_t bytes) __attribute__((malloc));

static void *small_alloc(size_t bytes)
{
    debug_record_arena_alloc(bytes);

    /* Assume sizeof(union align) is a power of 2 */
    union align { long l; double d; void *p; void (*fp)(void); };

    static void *buf;
    static size_t left = 0;
    void *ptr;
    struct new_perm_block *block;
    size_t alignment = sizeof(union align);

    assert(bytes > 0); /* rule out 0! */
    /* could sanity check upper bound here, but we're doing it below anyway */

    debug_fmt(DEBUG_CHANNEL_CONTROL, DEBUG_ENTER, SOURCE_LOCATION);

    /* round up bytes for proper alignment */
    bytes = ROUNDUP(bytes);

    /* do we not have enough left in the current block? */
    if (bytes > left)
    {
        debug_fmt(
            DEBUG_CHANNEL_MEMORY|DEBUG_CHANNEL_DETAIL,
            "%s: new block needed",
            SOURCE_LOCATION
        );

        /* allocate a new block */
        block = zero_malloc(ALLOCSIZE);
        debug_fmt(
            DEBUG_CHANNEL_MEMORY|DEBUG_CHANNEL_DETAIL,
            "%s: block @ %p",
            SOURCE_LOCATION,
            (void*) block
        );

        /* calculate bytes we have left */
        left = ALLOCSIZE - ROUNDUP(sizeof(block->next));

        /* check again if we have enough space */
        if (bytes > left)
        {
            panic_fmt(PANIC_SMALL_MEMORY, bytes, SOURCE_LOCATION);
        }

        /* insert at top of stack */
        block->next = new_permalloc_stack;
        new_permalloc_stack = block;

        /* setup buf to point to actual memory area */
        buf = ((char*)block) + ROUNDUP(sizeof(block->next)); /* char cast important! */
        debug_fmt(
            DEBUG_CHANNEL_MEMORY|DEBUG_CHANNEL_DETAIL,
            "%s: initial buf @ %p",
            SOURCE_LOCATION,
            (void*) buf
        );
    }

    ptr = buf;
    buf = ((char*)buf) + bytes; /* char cast important! */
    debug_fmt(
        DEBUG_CHANNEL_MEMORY|DEBUG_CHANNEL_DETAIL,
        "%s: adjusted buf @ %p",
        SOURCE_LOCATION,
        (void*) buf
    );
    assert(ptr < buf); /* TODO: good idea? [phf] */
    left -= bytes;
    debug_fmt(DEBUG_CHANNEL_CONTROL, DEBUG_LEAVE, SOURCE_LOCATION);
    return ptr;
}

/**
 * Free *all* memory allocated by small_alloc() so far.
 *
 * @warning This function should only be called if your process
 * is about to exit(3) or if you *really* know what you're doing;
 * otherwise you'll end up with lots of dangling pointers.
 */
static void small_free_all(void)
{
    /* the block we are about to free() */
    struct new_perm_block *current = NULL;

    debug_fmt(DEBUG_CHANNEL_CONTROL, DEBUG_ENTER, SOURCE_LOCATION);

    /* as long as we have block left */
    while (new_permalloc_stack != NULL)
    {
        /* remember the top block */
        current = new_permalloc_stack;
        /* pop the top block, stack possibly empty after this */
        new_permalloc_stack = current->next;
        /* free() the block we popped */
        debug_fmt(
            DEBUG_CHANNEL_MEMORY|DEBUG_CHANNEL_DETAIL,
            "%s: freed block @ %p",
            SOURCE_LOCATION,
            (void*) current
        );
        free(current);
    }

    debug_fmt(DEBUG_CHANNEL_CONTROL, DEBUG_LEAVE, SOURCE_LOCATION);
}
#endif /* _DASM_ARENA_ALLOC */

/*
 * [phf] Here's that badly hacked-together memory allocation
 * debugger. If you see a chance to clean it up, please do.
 */

#define DEBUG_MAX_ALLOC 128

struct debug_alloc_stat {
	size_t size;
	size_t count;
};

static int debug_num_regular;
static int debug_num_arena;

static struct debug_alloc_stat debug_regular[DEBUG_MAX_ALLOC];
static struct debug_alloc_stat debug_arena[DEBUG_MAX_ALLOC];

static int debug_find_stat_index(struct debug_alloc_stat *stats,
		int used, size_t bytes)
{
	for (int i = 0; i < used; i++) {
		if (stats[i].size == bytes) {
			return i;
		}
	}
	return -1;
}

static void debug_record_arena_alloc(size_t bytes)
{
	int i = debug_find_stat_index(debug_arena, debug_num_arena,
			bytes);
	if (i >= 0) {
		debug_arena[i].count++;
		return;
	}

	if  (debug_num_arena >= DEBUG_MAX_ALLOC) {
		debug_fmt(DEBUG_CHANNEL_INTERNAL,
			"Ran out of %d slots to track memory allocations.",
			DEBUG_MAX_ALLOC);
		return;
	}

	debug_arena[debug_num_arena].size = bytes;
	debug_arena[debug_num_arena].count = 1;
	debug_num_arena++;
}

static void debug_record_regular_alloc(size_t bytes)
{
	int i = debug_find_stat_index(debug_regular, debug_num_regular,
			bytes);
	if (i >= 0) {
		debug_regular[i].count++;
		return;
	}

	if (debug_num_regular >= DEBUG_MAX_ALLOC) {
		debug_fmt(DEBUG_CHANNEL_INTERNAL,
			"Ran out of %d slots to track memory allocations.",
			DEBUG_MAX_ALLOC);
		return;
	}

	debug_regular[debug_num_regular].size = bytes;
	debug_regular[debug_num_regular].count = 1;
	debug_num_regular++;
}

static void debug_memory_allocations(const char *title,
		struct debug_alloc_stat *stats, int used)
{
	size_t total_num;
	size_t total_size;

	total_num = total_size = 0;
	for (int i = 0; i < used; i++) {
		size_t count = stats[i].count;
		size_t size = stats[i].size;
		debug_fmt(DEBUG_CHANNEL_MEMORY,
				"%zu %s allocations of %zu bytes",
				count, title, size);
		total_num += count;
		total_size += count * size;
	}

	debug_fmt(DEBUG_CHANNEL_MEMORY,
		"Total of %zu %s allocations for %zu bytes (used %d slots)",
		total_num, title, total_size, used);
}

void debug_memory_allocation_patterns(void)
{
	debug_memory_allocations("regular", debug_regular,
			debug_num_regular);
	debug_memory_allocations("arena", debug_arena,
			debug_num_arena);
}

/* [phf] here's the actual API, ugly as hell ifdefs */

void *dalloc(size_t bytes)
{
#ifdef _DASM_ARENA_ALLOC
	return small_alloc(bytes);
#else
	return zero_malloc(bytes);
#endif
}

void dfree(void *address)
{
#ifdef _DASM_ARENA_ALLOC
	/* nothing to do, but avoid the GCC warning */
	address = address;
#else
	free(address);
#endif
}

void dfree_all(void)
{
#ifdef _DASM_ARENA_ALLOC
	small_free_all();
#else
	/* nothing to do */
#endif
}
