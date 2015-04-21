/*
	Simple framework for time-based micro benchmarks.

	I used to have some code here to compare, for example,
	the relative performance of % and &. This was written
	when I was playing with DASM's hash functions, I was
	trying to see if the old & optimization Matt did for
	the compression step was still necessary. It was. :-)

	In any case, the framework was rather crude: you had to
	tweak the number of times you wanted to run an operation
	by trial and error, and you only got time with "second"
	accuracy. Horrible.

	This is still crude, but it now uses alarm(2) to run
	whatever we want to benchmark for one second. So no
	more tweaking the count, we just perform the operation
	as often as possible in a second. Much nicer. I mostly
	rewrote it because I am now interested in how long we
	take for memory allocations.
*/

#define _POSIX_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <locale.h>
#include <signal.h>
#include <unistd.h>

/* The flag that tells us that one second is over. */
static volatile sig_atomic_t done = 0;

/*
	Some globals so the optimizer doesn't get ahead of us.
	Note that making these volatile is unfair to some of
	the functions we want to test, power-of-2 for example.
*/
static void *p = NULL;
static unsigned long v = 73;
static unsigned long w = 12;
static char *c = "test";
static bool b = false;

/*
	The functions/operations we're benchmarking. Obviously
	you have to ignore the overhead of the calling sequence
	but that's the reason for the "empty" test.
*/

static void empty(void)
{
	/* Nothing to do. This just measures overhead. */
}

static void tiny_malloc_free(void)
{
	p = malloc(48);
	/* We free right away to avoid making the host swap. */
	free(p);
}

static void small_malloc_free(void)
{
	p = malloc(1024);
	/* We free right away to avoid making the host swap. */
	free(p);
}

static void big_malloc_free(void)
{
	p = malloc(16384);
	/* We free right away to avoid making the host swap. */
	free(p);
}


static void tiny_calloc_free(void)
{
	p = calloc(1, 48);
	/* We free right away to avoid making the host swap. */
	free(p);
}

static void small_calloc_free(void)
{
	p = calloc(1, 1024);
	/* We free right away to avoid making the host swap. */
	free(p);
}

static void big_calloc_free(void)
{
	p = calloc(1, 16384);
	/* We free right away to avoid making the host swap. */
	free(p);
}

static void closest_power_of_two(void)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;
}

/*
	Added approximations of the actual hash functions that
	would be used. Seems that Dan's is a *tiny* bit slower
	(figures since there's an extra + in there) but in the
	actual program we have to weigh better distribution of
	hash values against that.
*/

static void dillon(void)
{
	v = (v << 2) + *c; /* ++ is missing :-/ */
}

static void bernstein(void)
{
	v = ((v << 5) + v) + *c; /* ++ is missing :-/ */
}

/*
	Here are those compression steps that I originally
	wanted to compare.
*/

static void mod(void)
{
	v = v % w;
}

static void and(void)
{
	v = v & w;
}

static void and_minus(void)
{
	v = v & (w-1);
}

static void power_of_two(void)
{
	b = (v != 0) && !(v & (v - 1));
}

/*
	Table of functions to benchmark. Obviously I've
	been hacking DASM for too long, I seem to use these
	kinds of "array of struct" things way too often
	these days. :-)
*/
static struct benchmark {
	void (*func)(void);
	char *name;
} benchmarks[] = {
	{empty, "empty"},
	{tiny_malloc_free, "tiny malloc"},
	{small_malloc_free, "small malloc"},
	{big_malloc_free, "big malloc"},
	{tiny_calloc_free, "tiny calloc"},
	{small_calloc_free, "small calloc"},
	{big_calloc_free, "big calloc"},
	{closest_power_of_two, "closest power-of-2"},
	{dillon, "dillon-hash"},
	{bernstein, "bernstein-hash"},
	{mod, "mod"},
	{and, "and"},
	{and_minus, "and-minus"},
	{power_of_two, "is power-of-2"},
	{NULL, NULL},
};

/* The signal handler. */
void handler(int signum)
{
	/* Trick to avoid warning. */
	done = signum - signum + 1;
}

/* Run a single benchmark. */
unsigned long run(void (*func)(void))
{
	unsigned long counter = 0;
	done = 0;
	alarm(1);
	while (!done) {
		func();
		counter++;
	}
	return counter;
}

/* And go... */
int main(void)
{
	struct sigaction action;
	action.sa_handler = handler;
	sigaction(SIGALRM, &action, NULL);

	setlocale(LC_NUMERIC, "");

	for (struct benchmark *b = benchmarks; b->name != NULL; b++) {
		unsigned long count = run(b->func);
		double inverted = (1.0 / count) * 1000 * 1000 * 1000;
		printf("%20s: %'13ld operations/second (%6g nanos each)\n", b->name, count, inverted);
	}

	return EXIT_SUCCESS;
}
