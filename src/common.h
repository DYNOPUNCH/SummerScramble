/*
 * common.h <SummerScramble>
 *
 * common helper functions, macros, and constants reside here
 *
 */

#ifndef SUMMER_COMMON_H_INCLUDED
#define SUMMER_COMMON_H_INCLUDED

/* display a fatal error message in a popup window */
void die(const char *fmt, ...);

/* print debug text to console */
void debug(const char *fmt, ...);

#define ARRAY_COUNT(X) (sizeof(X) / sizeof((X)[0]))

#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

#define MIN3(a,b,c)   MIN((a), MIN((b), (c)))
#define MAX3(a,b,c)   MAX((a), MAX((b), (c)))

/* colors in rgba8888 format */
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x000000ff
#define COLOR_PINK  0xffccccff

#endif /* SUMMER_COMMON_H_INCLUDED */

