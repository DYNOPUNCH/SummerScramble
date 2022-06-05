/* 
 * includeall.c <SyrupEngine>
 * 
 * generates a single .h file that #includes
 * all other .h files in a given directory and
 * its subdirectories
 * 
 */

#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ftw.h>

static const char *gPrefix = "";

static int ftw_callback(const char *fpath, const struct stat *sb, int typeflag)
{
	if (S_ISREG(sb->st_mode))
	{
		const char *ext = strrchr(fpath, '.');
		
		if (ext && !strcasecmp(ext, ".h"))
			fprintf(stdout, "#include \"%s%s\"\n", gPrefix, fpath + strlen("./"));
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
	gPrefix = argv[2];
	
	if (argc != 3 || !isDirectory(path))
	{
		fprintf(stderr, "args: includeall x z > out.h\n");
		fprintf(stderr, "where x is a directory containing headers\n");
		fprintf(stderr, "and z is the prefix to prepend to each\n");
		
		return -1;
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

