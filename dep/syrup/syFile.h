/*
 * syFile.h <SyrupEngine>
 *
 * rudimentary file handling + virtual filesystems
 */

#ifndef SYRUP_FILE_H_INCLUDED
#define SYRUP_FILE_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

struct syFileVirtual
{
	const char *filename;
	const unsigned char *data;
	size_t size;
};

void syFileFree(const void *data);
const void *syFileLoadReadonly(const char *fn, size_t *sz, bool *isCopy);
void *syFileLoad(const char *fn, size_t *sz);
void syFileUseVFS(const struct syFileVirtual *array, int num);

#endif /* SYRUP_FILE_H_INCLUDED */

