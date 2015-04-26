/*
 * Just some code thrown together to try out a few C11 features.
 */

#include <assert.h>
#include <stdalign.h>
#include <stdnoreturn.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Static assertions mean we can finally check compile-time
 * requirements without cpp #if #endif hacks. Note that the
 * message is required. Take out the "+1" and see it fail.
 */
static_assert(10 == 9+1, "weird");

/*
 * I thought we'd need some C11 thingy to make UTF-8 work
 * but I guess not? Cannot figure out UTF-8 chars though...
 */

const char *str = "⌘";
const char *str1 = u8"⌘";

/*
 * We can finally mark functions as not returning without
 * having to rely on __attribute((noreturn))__ extensions.
 */
noreturn
void doit(void)
{
	exit(EXIT_SUCCESS);
	/* If you take out the exit, you'll get a warning here. */
}

int pad(void)
{
	doit();
	/* This is where we'd get a warning without noreturn. */
}

int main(void)
{
	/* Sadly gcc 4.8.4 doesn't have max_align_t defined. */
	/*printf("max align = %d\n", alignof(max_align_t));*/
	printf("max align = %zu\n", alignof(double));

	printf("a string = %s\n", str);
	printf("a string = %s\n", str1);

	pad();
}
