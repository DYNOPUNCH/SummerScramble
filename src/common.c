/*
 * common.c <SummerScramble>
 *
 * common helper functions, macros, and constants reside here
 *
 */

#include <SDL2/SDL.h> /* needed only forSDL_ShowSimpleMessageBox */
#include <stdio.h>
#include <stdarg.h>
#include "common.h"

/* display a fatal error message in a popup window */
void die(const char *fmt, ...)
{
	char buf[4096];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	fprintf(stderr, "%s\n", buf);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", buf, 0);
	va_end(args);
	
	exit(EXIT_FAILURE);
}

/* print debug text to console */
void debug(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fflush(stderr); /* necessary for msys2 console output */
}
