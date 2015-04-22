#ifndef _DASM_DALLOC_H
#define _DASM_DALLOC_H

#include <stdlib.h>

/**
 * @file
 *
 * The (revised) DASM memory allocator.
 *
 * For the longest time DASM used *two* allocators in parallel, a thin wrapper
 * around malloc(3) and a specialized arena-style allocator. The intent was,
 * apparently, to allocate small objects using the specialized allocator while
 * larger objects would (essentially) use malloc(3)/free(3). (However, the
 * name of the original allocator, permalloc, might also indicate the goal of
 * keeping those objects around longer than others.)
 *
 * We ran quite a few benchmarks regarding the memory allocation patterns
 * exhibited by DASM in April 2015 and found that *most* allocations are for
 * objects less than 64 bytes in size (many are a good bit smaller, all the
 * way down to 2 bytes in fact). Note that this was true for *both* allocators:
 * The only time malloc(3) was used for a "large" amount of memory (16kB) was
 * by the arena-style allocator!
 *
 * Since performance is no longer a serious concern for DASM, we could have
 * removed the specialized allocator and used malloc(3) for everything.
 * However, the arena-style allocator has *one* wonderful property *beyond*
 * performance: It's trivial to free memory correctly by destroying the arenas.
 * In other words, if we switched to the arena-style allocator exclusively, all
 * the remaining memory leaks would go away. Also, because all objects
 * allocated seem to be well below 16kB, the fixed-size arena is very unlikely
 * to become a problem.
 *
 * However, the arena-style allocator also suffers from a *big* drawback: Tools
 * like valgrind(1) cannot warn us about illegal memory accesses inside the
 * arenas. In light of this it would be better to switch to the thin malloc(3)
 * wrapper exclusively.
 *
 * Eventually we decided that we want both options available. By default we use
 * the arena-based allocator for releases and the malloc(3)-based allocator for
 * development. The API we define here is lose enough to apply to both.
 * A compile-time flag decides which one gets used.
 *
 * @warning Please do *not* add code to DASM that allocates/frees memory in
 * some other way. All of DASM should use this API for memory allocation.
 */

/**
 * Allocate memory for an object of the given size.
 *
 * If dalloc() cannot allocate the memory required, it calls panic_fmt() and
 * exits, thus eliminating the need to check the return value over and over
 * again.
 *
 * @pre bytes > 0
 *
 * @warning You cannot free(3) the pointer returned by dalloc(), bad things
 * will happen if you try. Use dfree() or dfree_all() instead.
 */
void *dalloc(size_t bytes) __attribute((malloc));

/**
 * Suggest freeing memory at the given address.
 *
 * The dfree() call provides a hint to the memory allocator that the given
 * block will no longer be used and should be freed. Whether it will actually
 * be freed, and when, is undefined.
 *
 * @warning The pointer you pass to dfree() *must* be from a previous call to
 * dalloc(), bad things will happen if you pass a pointer obtained elsewhere.
 */
void dfree(void *address);

/**
 * Free *all* memory allocated by dalloc() so far.
 *
 * @warning This function should only be called if your process is about to
 * exit(3) or if you *really* know what you're doing; otherwise you'll end
 * up with lots of dangling pointers.
 */
void dfree_all(void);

/**
 * Debugging output for memory allocation patterns.
 */
void debug_memory_allocation_patterns(void);

#endif /* _DASM_DALLOC_H */
