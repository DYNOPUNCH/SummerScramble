/* 
 * vfsgen.c <SyrupEngine>
 * 
 * generates a simple virtual filesystem
 * meant to be used with syFile
 * 
 */

#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <ftw.h>

static char **excludePaths = 0;
static int excludePathsCount = 0;
static char **excludePatterns = 0;
static int excludePatternsCount = 0;

static void addString(char ***list, int *count, char *v)
{
	*list = realloc(*list, (*count + 1) * sizeof(*list));
	assert(*list);
	
	(*list)[*count] = v;
	*count += 1;
}

static bool isExcludedPath(const char *fpath)
{
	int myLen = strlen(fpath);
	int i;
	
	for (i = 0; i < excludePathsCount; ++i)
	{
		const char *p = excludePaths[i];
		int pLen = strlen(p);
		int imin = pLen < myLen ? pLen : myLen;
		
		if (pLen > myLen)
			continue;
		
		if (!memcmp(p, fpath,  imin))
			return true;
	}
	
	return false;
}

static bool isExcludedPattern(const char *fpath)
{
	int i;
	
	for (i = 0; i < excludePatternsCount; ++i)
	{
		if (strstr(fpath, excludePatterns[i]))
			return true;
	}
	
	return false;
}

static bool isExcluded(const char *fpath)
{
	return isExcludedPattern(fpath) || isExcludedPath(fpath);
}

static int ftw_callback(const char *fpath, const struct stat *sb, int typeflag)
{
	fpath += strlen("./");
	
	if (isExcluded(fpath))
		return 0;
	
	if (S_ISREG(sb->st_mode))
	{
		FILE *fp;
		int size = 0;
		int c;
		
		fprintf(stderr, "including file '%s'\n", fpath);
		
		fp = fopen(fpath, "rb");
		if (!fp)
		{
			fprintf(stderr, "failed to open '%s' for reading\n", fpath);
			return -1;
		}
		
		fprintf(stdout, "{ \"%s\", (unsigned char[]){", fpath);
		while ((c = fgetc(fp)) != EOF /*&& size < 16*/)
		{
			fprintf(stdout, "%d,", c);
			++size;
		}
		
		/* append a zero terminator in case file is text */
		fprintf(stdout, "0");
		++size;
		
		fprintf(stdout, "}, %d", size);
		fprintf(stdout, "},\n");
	}
	
	(void)typeflag;
	
	return 0;
}

static int handleDirectory(const char *path)
{
	if (chdir(path))
	{
		fprintf(stderr, "chdir error\n");
		return -1;
	}
	if (ftw(".", ftw_callback, 0))
	{
		fprintf(stderr, "ftw error\n");
		return -1;
	}
	
	return 0;
}

static bool isDirectory(const char *path)
{
	struct stat buf;
	
	if (stat(path, &buf) != 0)
		return false;
	
	return S_ISDIR(buf.st_mode) != 0;
}

int main(int argc, char *argv[])
{
	const char *path = argv[1];
	int i;
	
	if (argc < 2 || !isDirectory(path))
	{
		fprintf(stderr, "args: vfsgen basedir -x a -x b > out.h\n");
		fprintf(stderr, "where basedir is a directory containing files\n");
		fprintf(stderr, "and subdirectories we prefer to include\n");
		fprintf(stderr, "  -x   is used to specify subdirectories to exclude\n");
		fprintf(stderr, "       and can be used multiple times\n");
		fprintf(stderr, "  -xp  is used to specify patterns to exclude\n");
		fprintf(stderr, "       and can be used multiple times\n");
		
		return -1;
	}
	
	for (i = 2; i < argc; i += 2)
	{
		char *this = argv[i];
		char *next = argv[i + 1];
		
		if (!strcasecmp(this, "-x"))
			addString(&excludePaths, &excludePathsCount, next);
		else if (!strcasecmp(this, "-xp"))
			addString(&excludePatterns, &excludePatternsCount, next);
		else
		{
			fprintf(stderr, "unknown argument '%s'\n", this);
			return -1;
		}
	}
	
	if (handleDirectory(path))
	{
		fprintf(stderr, "failed to process '%s'; does it exist?\n"
			"is it a file? is it a directory?\n"
			, path
		);
		return -1;
	}
	
	fprintf(stdout, "\n");
	
	return 0;
}

