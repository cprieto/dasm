#ifndef _DASM_PLATFORM_H
#define _DASM_PLATFORM_H

/**
 * @file
 *
 * Dealing with platform issues for hosts and targets.
 *
 * The "host" is the platform we build DASM on; the issues we deal with
 * relate mostly to the compiler and library used.
 *
 * The "target" is the platform we want DASM to run on; the issues we deal
 * with relate mostly to filesystem paths for now.
 *
 * @todo We use predefined preprocessor macros/symbols for all of this, but
 * those might have changed over time. If you're in one of the environments
 * we don't usually have access to (Apple and Windows) you could help us out
 * by running "cpp -dM </dev/null" or an equivalent command and telling us
 * what we should check for to remain portable. Thanks!
 *
 * @todo There are many parts of DASM where even the simple things available
 * here are not yet used, the INCDIR directive comes to mind for instance.
 * It's a work in progress.
 */

/*
 * Compilers on Windows (Visual Studio in particular) seem to require a bit
 * of extra work.
 */

#if defined(__WINDOWS__) || defined(_WIN32) || defined(_WIN64)
	/* no stdbool.h so fake it */
#	define bool int
#	define false (0==1)
#	define true (!false)
	/* no strcasecmp so fake it using stricmp */
#	define strcasecmp stricmp
#else
#	include <stdbool.h>
#	include <strings.h>
#endif

/*
 * The GNU C compiler allows for a bunch of annotations that DASM makes use
 * of. Sometimes we can provide alternatives for other compilers/tools,
 * sometimes not.
 */

/*
 * This gets rid of ALL __attribute__ stuff for non-GNU compilers.
 */
#if !defined(__GNUC__)
#	define __attribute__(x)  /* GNU C __attribute__ removed */
#endif

/*
 * Martin Pool's cool little macro to "portably" use various "unused"
 * annotations. Good for GNU C and splint so far.
 */

#if defined(UNUSED)
	/* already defined, do nothing */
#elif defined(__GNUC__)
#	define UNUSED(x) x ## _UNUSED __attribute__((unused))
#elif defined(__LCLINT__) || defined(S_SPLINT_S)
#	define UNUSED(x) /*@unused@*/ x
#else
#	define UNUSED(x) x
#endif

/*
 * Here's another that goes in front of functions that don't return.
 */

#ifdef NORETURN
	/* already defined, do nothing */
#elif defined(__GNUC__)
#	define NORETURN __attribute__((noreturn))
#elif defined(__LCLINT__) || defined(S_SPLINT_S)
#	define NORETURN /*@noreturn@*/
#else
#	define NORETURN /* does not return */
#endif

/*
 * We need to deal with filesystem paths in various places. Focusing on the
 * path separator is far from perfect, but it's better than nothing. We still
 * need to find a way to distinguish absolute paths from relative ones, and
 * to deal with parent directories.
 */

#if defined(__WINDOWS__) || defined(_WIN32) || defined(_WIN64)
#	define DASM_PATH_SEPARATOR '\\'
#elif defined(__unix__) || defined(__linux__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__amigaos__)
#	define DASM_PATH_SEPARATOR '/'
#else
#	error No path separator for your platform!
#endif

#endif /* _DASM_PLATFORM_H */
