/*
 * syFile.c <SyrupEngine>
 *
 * rudimentary file handling + virtual filesystems
 */

#include "syFile.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

static struct
{
	struct
	{
		const struct syFileVirtual *array;
		int num;
	} vfs;
} g = {0};

void syFileUseVFS(const struct syFileVirtual *array, int num)
{
	g.vfs.array = array;
	g.vfs.num = num;
}

/* XXX only use this when syFileLoadReadonly sets isCopy = true,
 *     or on syFileLoad's output
 */
void syFileFree(const void *data)
{
	free((void*)data);
}

const void *syFileLoadReadonly(const char *fn, size_t *sz, bool *isCopy)
{
	FILE *fp;
	void *data;
	size_t sz_;
	
	*isCopy = false;
	
	if (g.vfs.array)
	{
		const struct syFileVirtual *file;
		int i;
		
		for (file = g.vfs.array, i = 0; i < g.vfs.num; ++i, ++file)
		{
			if (!strcmp(file->filename, fn))
			{
				if (sz)
					*sz = file->size;
				return file->data;
			}
		}
		
		fprintf(stderr, "failed to locate file '%s'\n", fn);
		return 0;
	}
	
	if (!sz)
		sz = &sz_;
	
	fp = fopen(fn, "rb");
	if (!fp)
	{
		fprintf(stderr, "failed to open file '%s' for reading\n", fn);
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	*sz = ftell(fp);
	data = malloc(*sz);
	assert(data);
	fseek(fp, 0, SEEK_SET);
	if (fread(data, 1, *sz, fp) != *sz)
	{
		fprintf(stderr, "read error on file '%s'\n", fn);
		return 0;
	}
	
	*isCopy = true;
	return data;
}

void *syFileLoad(const char *fn, size_t *sz)
{
	const void *data;
	void *out;
	size_t sz_;
	bool isCopy;
	
	if (!sz)
		sz = &sz_;
	
	data = syFileLoadReadonly(fn, sz, &isCopy);
	
	if (!data)
		return 0;
	
	/* is already a copy of the data, so no need to make our own clone */
	if (isCopy)
		return (void*)data;
	
	/* otherwise, return a copy that is safe to modify */
	out = malloc(*sz);
	assert(out);
	memcpy(out, data, *sz);
	
	return out;
}

