/* 
 * ogmo2c.c <SyrupEngine>
 * 
 * converts Ogmo JSON levels to C
 *
 * you need libcjson-dev in order to build this
 * 
 */
 
#include <cjson/cJSON.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ftw.h>
#include <ctype.h>

#define DIE(...) { fprintf(stderr, __VA_ARGS__); return -1; }
#define ERRPREFIX " -> "

#define UNUSED___ "unused___"

#define INCLUDE_LEFT   "<"
#define INCLUDE_RIGHT  ">"

#define SIG_NO_QUOTES "\x0f\x0e\x0d\x0c\x0b\x0a\x09\x08\x07\x06\x05\x04\x03"

#define KIB * 1024
#define MIB * 1024 * 1024

#define SWITCH_STR_BEGIN(X) { const char *SWITCH_STR_WORK = X;
#define SWITCH_STR_FIRSTCASE(X) if (!strcmp(SWITCH_STR_WORK, X))
#define SWITCH_STR_CASE(X) else SWITCH_STR_FIRSTCASE(X)
#define SWITCH_STR_UNKNOWN else
#define SWITCH_STR_END }

#define FIRSTCASE(X) if (!strcmp(now->string, X))
#define CASE(X) else FIRSTCASE(X)
#define UNKNOWN else
	
const char *gEntityNamePrefix = "En_";
const char *gTabStyle = " ";
FILE *gOut;

/* TODO organization, cleanup, and safety */

static char *myStrdup(const char *src)
{
	char *dst;
	int srcLen;
	
	if (!src)
		return 0;
	
	srcLen = strlen(src) + 1;
	
	dst = malloc(srcLen);
	assert(dst);
	memcpy(dst, src, srcLen);
	
	return dst;
}

static char *skipQuote(char *s)
{
	assert(s);
	
	while (*s)
	{
		++s;
		if (*s == '"' && *(s-1) != '\\')
			break;
	}
	
	return s;
}

static void removeComments(char *s)
{
	assert(s);
	
	while (*s)
	{
		if (*s == '"')
			s = skipQuote(s);
		else if (!memcmp(s, "//", 2))
			memset(s, ' ', strcspn(s, "\r\n"));
		else if (!memcmp(s, "#if 0", 5))
		{
			char *end = strstr(s, "#endif");
			if (!end)
				end = s + strlen(s);
			else
				end += strlen("#endif");
			memset(s, ' ', end - s);
		}
		else if (!memcmp(s, "/*", 2))
		{
			char *end = strstr(s, "*/");
			if (!end)
				end = s + strlen(s);
			else
				end += strlen("*/");
			memset(s, ' ', end - s);
		}
		
		++s;
	}
}

static void condenseWhitespace(char *s, int maxContinuousSpaces)
{
	assert(s);
	
	while (*s)
	{
		int n;
		
		if (*s == '"')
		{
			s = skipQuote(s);
			++s;
			continue;
		}
		
		n = strspn(s, " ");
		
		if (n > maxContinuousSpaces)
		{
			memmove(s + maxContinuousSpaces, s + n, strlen(s + n) + 1);
			continue;
		}
		++s;
	}
}

struct StringRegion
{
	char *begin;
	char *end;
	char *name;
	int length;
};

static void strcatf(char *dst, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsprintf(dst + strlen(dst), fmt, args);
	va_end(args);
}

static struct StringRegion StringRegionFind(
	char *str
	, const char *begin
	, const char *end
	, const char *isSubregion
)
{
	struct StringRegion result = {0};
	
	assert(str);
	assert(begin);
	assert(end);
	
	result.begin = strstr(str, begin);
	result.end = strstr(str, end);
	if (!result.begin || !result.end)
	{
		const char *which = !result.begin ? begin : end;
		if (isSubregion)
			fprintf(
				stderr
				, ERRPREFIX "failed to locate '%s' within subregion '%.*s'\n"
				, which
				, (int)strcspn(isSubregion, "\r\n")
				, isSubregion
			);
		else
			fprintf(stderr, ERRPREFIX "failed to locate '%s' within file\n", which);
		exit(EXIT_FAILURE);
	}
	else if (result.end < result.begin)
	{
		fprintf(stderr, ERRPREFIX "'%s' and '%s' are in reverse order...\n", begin, end);
		exit(EXIT_FAILURE);
	}
	
	result.name = calloc(1, strcspn(result.begin, "\r\n") + 1);
	memcpy(result.name, result.begin, strcspn(result.begin, "\r\n"));
	
	/* old logic (includes start/end tags) */
	//result.length = (result.end - result.begin) + strlen(end);
	
	/* new logic (excludes start/end tags) */
	result.begin += strlen(begin);
	result.length = result.end - result.begin;
	
	return result;
}

static struct StringRegion OgmoRegion_(char *str, char *tok, const char *isSubregion)
{
	char begin[512];
	char end[512];
	assert(str);
	assert(tok);
	
	snprintf(begin, sizeof(begin), "/* <%s> */", tok);
	snprintf(end, sizeof(end), "/* </%s> */", tok);
	
	return StringRegionFind(str, begin, end, isSubregion);
}

static struct StringRegion OgmoRegion(char *str, char *tok)
{
	return OgmoRegion_(str, tok, false);
}

static struct StringRegion OgmoRegionInRegion(struct StringRegion haystack, char *tok)
{
	struct StringRegion needle = OgmoRegion_(haystack.begin, tok, haystack.name);
	bool beginOOB = false;
	bool endOOB = false;
	
	if (!needle.begin
		|| (beginOOB = (needle.begin >= haystack.begin + haystack.length))
		|| (endOOB = (needle.end >= haystack.begin + haystack.length))
	)
	{
		fprintf(
			stderr
			, "failed to locate '/* <%s%s> */' inside '%.*s'\n"
			, endOOB ? "/" : ""
			, tok
			, (int)strcspn(haystack.begin, "\r\n")
			, haystack.begin
		);
		exit(EXIT_FAILURE);
	}
	
	return needle;
}

static bool OgmoRegionReplace(struct StringRegion reg, const char *str)
{
	int strLen;
	int regLen;
	
	assert(str);
	assert(reg.begin);
	
	strLen = strlen(str);
	regLen = strlen(reg.begin);
	
	/* no change */
	//fprintf(stderr, "compare '%.10s' vs '%.10s'\n", reg.begin, str);
	if (regLen >= strLen && !memcmp(str, reg.begin, strLen))
		return false;
	
	memmove(reg.begin + strLen, reg.end, strlen(reg.end) + 1);
	
	memcpy(reg.begin, str, strLen);
	
	return true;
}

static const char *ifndef(const char *str)
{
	static char buf[4096];
	char *b;
	
	strcpy(buf, "OGMO2C_");
	for (b = buf + strlen(buf); *str; ++str, ++b)
		*b = isalnum(*str) ? toupper(*str) : '_';
	
	return buf;
}

static void gOut_redirect(const char *fn)
{
	if (!fn)
	{
		fprintf(gOut, "\n""#endif\n\n");
		fclose(gOut);
		gOut = stdout;
		return;
	}
	
	else if (!(gOut = fopen(fn, "w")))
	{
		fprintf(stderr, "failed to open '%s' for writing\n", fn);
		exit(EXIT_FAILURE);
	}
	
	fprintf(
		gOut
		, "/*\n *\n * DO NOT EDIT THIS FILE; IT IS AUTOGENERATED BY\n"
		" * ogmo2c AND YOUR CHANGES WILL BE OVERWRITTEN!\n *\n */\n\n"
		"#ifndef %s\n"
		"#define %s\n\n"
		, ifndef(fn)
		, ifndef(fn)
	);
}

struct Vec2f
{
	int x;
	int y;
};

struct Vec2i
{
	int x;
	int y;
};

struct OgmoEntityValueDef
{
	char *name;
	char *definition;
	char *definition_c; /* extra */
	int display;
	char *defaultsAsString; /* extra */
	bool hasOverride; /* extra */
	union
	{
		struct
		{
			bool defaults;
		} Boolean;
		struct
		{
			char *defaults;
			bool includeAlpha;
		} Color;
		struct
		{
			char **choices;
			int defaults;
		} Enum;
		struct
		{
			char *defaults;
			char **extensions;
		} Filepath;
		struct
		{
			float defaults;
			bool bounded;
			float min;
			float max;
		} Float;
		struct
		{
			int defaults;
			bool bounded;
			int min;
			int max;
		} Integer;
		struct
		{
			char *defaults;
			int maxLength;
			bool trimWhitespace;
		} String;
		struct
		{
			char *defaults;
		} Text;
	} u;
};

struct OgmoEntityDef
{
	char *exportID;
	char *name;
	int limit;
	struct Vec2f size;
	struct Vec2f origin;
	bool originAnchored;
	void *shape; /* TODO */
	char *color;
	bool tileX;
	bool tileY;
	struct Vec2f tileSize;
	bool resizeableX;
	bool resizeableY;
	bool rotatable;
	float rotationDegrees;
	bool canFlipX;
	bool canFlipY;
	bool canSetColor;
	bool hasNodes;
	int nodeLimit;
	int nodeDisplay;
	bool nodeGhost;
	char **tags;
	struct OgmoEntityValueDef *values;
	int valuesCount; /* extra */
	char *valuesProgrammer; /* extra */
	const char *texture;
};

struct OgmoProject
{
	char *name;
	char *ogmoVersion;
	char **levelPaths;
	char *backgroundColor;
	char *gridColor;
	bool anglesRadians;
	int directoryDepth;
	struct Vec2i layerGridDefaultSize;
	struct Vec2i levelDefaultSize;
	struct Vec2i levelMinSize;
	struct Vec2i levelMaxSize;
	char **levelValues;
	char *defaultExportMode;
	bool compactExport;
	char *externalScript;
	char *playCommand;
	char **entityTags;
	struct OgmoEntityDef *entities;
	int entitiesCount; /* extra */
	void *layers; /* TODO */
	void *tilesets; /* TODO */
	char *relpath; /* extra */
};

static struct OgmoProject *gOgmoProject = 0;

/* minimal file loader
 * returns 0 on failure
 * returns pointer to loaded file on success
 * a zero terminator is added
 */
char *loadfilestring(const char *fn)
{
	FILE *fp = 0;
	char *dat;
	size_t sz;
	
	/* rudimentary error checking returns 0 on any error */
	if (
		!fn
		|| !(fp = fopen(fn, "rb"))
		|| fseek(fp, 0, SEEK_END)
		|| !(sz = ftell(fp))
		|| fseek(fp, 0, SEEK_SET)
		|| !(dat = malloc(sz+1))
		|| fread(dat, 1, sz, fp) != sz
		|| fclose(fp)
	)
		return 0;
	
	dat[sz] = '\0';
	return dat;
}

void writefilestring(const char *fn, const char *string)
{
	FILE *fp;
	size_t len;
	bool exists = false;
	
	assert(fn);
	assert(string);
	
	len = strlen(string);
	
	assert(len);
	
	fp = fopen(fn, "rb");
	if (fp)
	{
		exists = true;
		fclose(fp);
	}
	
	fp = fopen(fn, "wb");
	if (!fp)
	{
		fprintf(stderr, "failed to open '%s' for writing\n", fn);
		exit(EXIT_FAILURE);
	}
	if (fwrite(string, 1, len, fp) != len)
	{
		size_t i;
		
		/* failed to write file all at once, retry */
		fclose(fp);
		
		fp = fopen(fn, "wb");
		if (!fp)
		{
			fprintf(stderr, "failed to open '%s' for writing\n", fn);
			exit(EXIT_FAILURE);
		}
		
		for (i = 0; i < len; ++i)
		{
			if (fputc(string[i], fp) == EOF)
			{
				fprintf(stderr, "error writing '%s'!\n", fn);
				if (exists)
					fprintf(
						stderr,
						"furthermore, the original may now be corrupt!\n"
						"hope you have a backup!\n"
					);
				exit(EXIT_FAILURE);
			}
		}
	}
}

char *loadfilestringFast(const char *fn, size_t *bufSz)
{
	FILE *fp = 0;
	static char *dat = 0;
	static size_t datSz = 16 MIB;
	size_t sz;
	
	/* cleanup routine */
	if (!fn)
	{
		if (dat)
			free(dat);
		dat = 0;
		return 0;
	}
	
	if (!dat)
	{
		dat = malloc(datSz);
		assert(dat);
	}
	
	/* empty buffer request */
	if (!fn[0])
	{
		if (bufSz)
			*bufSz = datSz;
		return strcpy(dat, "");
	}
	
	/* rudimentary error checking returns 0 on any error */
	if (
		!fn
		|| !(fp = fopen(fn, "rb"))
		|| fseek(fp, 0, SEEK_END)
		|| !(sz = ftell(fp))
		|| fseek(fp, 0, SEEK_SET)
	)
		return 0;
	
	/* it is assumed that there is room for an extra copy at the end */
	if (datSz < sz * 2)
	{
		datSz = sz * 4;
		
		dat = realloc(dat, datSz);
		assert(dat);
	}
	
	if (fread(dat, 1, sz, fp) != sz)
	{
		fprintf(stderr, "file read error '%s'\n", fn);
		exit(EXIT_FAILURE);
	}
	
	fclose(fp);
	
	if (bufSz)
		*bufSz = datSz;
	dat[sz] = '\0';
	return dat;
}

void OgmoEntityDefClearValueOverrides(struct OgmoEntityDef *entity)
{
	struct OgmoEntityValueDef *v;
	
	assert(entity);
	
	for (v = entity->values; v < entity->values + entity->valuesCount; ++v)
		v->hasOverride = false;
}

struct OgmoEntityDef *OgmoEntityDefFindByName(struct OgmoProject *ogmo, const char *name)
{
	struct OgmoEntityDef *en;
	
	if (!ogmo)
		ogmo = gOgmoProject;
	
	assert(ogmo);
	
	for (en = ogmo->entities; en < ogmo->entities + ogmo->entitiesCount; ++en)
	{
		if (!strcmp(en->name, name))
		{
			OgmoEntityDefClearValueOverrides(en);
			return en;
		}
	}
	
	fprintf(stderr, "unknown entity '%s'\n", name);
	exit(EXIT_FAILURE);
	return 0;
}

static struct OgmoEntityValueDef *OgmoEntityDefFindValue(struct OgmoEntityDef *entity, const char *valuename)
{
	struct OgmoEntityValueDef *v;
	
	assert(entity);
	
	for (v = entity->values; v < entity->values + entity->valuesCount; ++v)
	{
		if (!strcmp(v->name, valuename))
			return v;
	}
	
	return 0;
}

static void OgmoEntityDefSetValueOverride(struct OgmoEntityDef *entity, const char *valuename)
{
	struct OgmoEntityValueDef *v = OgmoEntityDefFindValue(entity, valuename);
	
	assert(entity);
	
	if (!v)
		return;
	
	v->hasOverride = true;
}

static bool isDirectory(const char *path)
{
	struct stat buf;
	
	if (stat(path, &buf) != 0)
		return false;
	
	return S_ISDIR(buf.st_mode) != 0;
}

static bool isFile(const char *path)
{
	struct stat buf;
	
	if (stat(path, &buf) != 0)
		return false;
	
	return S_ISREG(buf.st_mode) != 0;
}

static const char *doRoomPath(const char *path)
{
	static char newpath[4096];
	const char *ss;
	
	assert(path);
	
	ss = strstr(path, "room/");
	if (!ss)
		ss = path;
	else
		ss += strlen("room/");
	
	snprintf(newpath, sizeof(newpath), "%.*s", (int)strcspn(ss, "."), ss);
	
	return newpath;
}

static void valuedouble(FILE *dst, const double src)
{
	if (fmod(fabs(src), 1) < 0.00001)
		fprintf(dst, "%.lf", src);
	else
		fprintf(dst, "%lf", src);
}

static void printIndent(const int indent)
{
	int i;
	
	for (i = 0; i < indent; ++i)
		fprintf(gOut, "%s", gTabStyle);
}

static char *sanitizeNewlines(const char *str)
{
	const char *src;
	char *dst;
	char *result;
	
	if (!str)
		return 0;
	
	/* 8x as many characters to account for expansion */
	result = malloc(strlen(str) * 8 + 1);
	assert(result);
	
	for (src = str, dst = result; *src; ++src, ++dst)
	{
		if (*src == '\n')
		{
			*dst = '\\';
			++dst;
			*dst = 'n';
			++dst;
			*dst = '"';
			++dst;
			*dst = '"';
		}
		else
			*dst = *src;
	}
	
	*dst = '\0';
	
	return result;
}

static void PrintJSON(cJSON *json)
{
	static int indent = 1;
	static const char *nameLast = "";
	
	if (!json)
		return;
	
	if (json->string)
	{
		printIndent(indent);
		
		fprintf(gOut, ".%s = ", json->string);
		if (cJSON_IsArray(json) || cJSON_IsObject(json))
		{
			cJSON *walk;
			int arrayCount = 0;
			struct OgmoEntityDef *enDef = 0;
			
			if (!strcmp(json->string, "layers"))
				fprintf(gOut, "(struct syOgmoLayer[])");
			else if (!strcmp(json->string, "entities"))
				fprintf(gOut, "(struct syOgmoEntity[])");
			else if (!strcmp(json->string, "values"))
			{
				fprintf(gOut, "&(struct %s%s%s)", gEntityNamePrefix, nameLast, "");
				enDef = OgmoEntityDefFindByName(0, nameLast);
			}
			else if (!strcmp(json->string, "decals"))
				fprintf(gOut, "(struct syOgmoDecal[])");
			fprintf(gOut, "\n");
			printIndent(indent);
			fprintf(gOut, "{\n");
			
			indent += 1;
				if (cJSON_IsArray(json))
				{
					for (walk = json->child; walk; walk = walk->next, ++arrayCount)
					{
						printIndent(indent);
							fprintf(gOut, "{\n");
								indent += 1;
									PrintJSON(walk->child);
								indent -= 1;
							printIndent(indent);
						fprintf(gOut, "},\n");
					}
				}
				else
				{
					if (enDef)
					{
						//fprintf(stderr, "%s\n", json->string);
						for (walk = json->child; walk; walk = walk->next)
						{
							struct OgmoEntityValueDef *v;
							
							v = OgmoEntityDefFindValue(enDef, walk->string);
							if (v)
							{
								if (!strcasecmp(v->definition, "color"))
								{
									char buf[1024];
									
									snprintf(
										buf
										, sizeof(buf)
										, "0x%s" SIG_NO_QUOTES
										, walk->valuestring + 1
									);
									
									walk->valuestring = myStrdup(buf);
								}
								else if (!strcasecmp(v->definition, "enum"))
								{
									char buf[1024];
									
									/* enum name */
									snprintf(
										buf
										, sizeof(buf)
										, "%s%s%s_%s"
										, gEntityNamePrefix
										, enDef->name
										, v->name
										, walk->valuestring
									);
									
									condenseWhitespace(buf, 0);
									
									strcat(buf, SIG_NO_QUOTES);
									
									walk->valuestring = myStrdup(buf);
								}
							}
							OgmoEntityDefSetValueOverride(enDef, walk->string);
							//fprintf(stderr, " -> %s\n", walk->string);
						}
					}
					PrintJSON(json->child);
				}
				/* handle unset defaults */
				if (enDef)
				{
					struct OgmoEntityValueDef *v;
					
					for (v = enDef->values; v < enDef->values + enDef->valuesCount; ++v)
						if (!v->hasOverride)
						{
							printIndent(indent);
							fprintf(gOut, ".%s = %s,\n", v->name, v->defaultsAsString);
						}
					
					printIndent(indent);
					fprintf(gOut, "%s\n", enDef->valuesProgrammer);
				}
			indent -= 1;
			
			printIndent(indent);
			fprintf(gOut, "}");
			
			if (arrayCount)
			{
				fprintf(gOut, ",\n");
				printIndent(indent);
				fprintf(gOut, ".%sCount = %d", json->string, arrayCount);
			}
		}
		else if (cJSON_IsString(json))
		{
			char *vs = json->valuestring;
			char *ss;
			
			if ((ss = strstr(vs, SIG_NO_QUOTES)))
			{
				fprintf(gOut, "%.*s", (int)(ss - vs), vs);
			}
			else
				fprintf(gOut, "\"%s\"", sanitizeNewlines(vs));
			
			if (!strcmp(json->string, "name"))
				nameLast = vs;
		}
		else if (cJSON_IsNumber(json))
			valuedouble(gOut, json->valuedouble);
		else if (cJSON_IsObject(json))
			fprintf(gOut, "object");
		else if (cJSON_IsBool(json))
			fprintf(gOut, cJSON_IsTrue(json) ? "true" : "false");
		else
			fprintf(gOut, "unknown(%d)", json->type & 0xff);
		fprintf(gOut, ",\n");
		
		if (!strcmp(json->string, "values"))
		{
			printIndent(indent);
			fprintf(gOut, ".valuesClass = %s%s,\n", gEntityNamePrefix, nameLast);
		}
	}
	PrintJSON(json->next);
}

static int handleFile(const char *path, bool isOne)
{
	char *contents = loadfilestring(path);
	cJSON *json;
	
	if (!contents)
		DIE("failed to load file '%s'\n", path);
	
	json = cJSON_Parse(contents);
	if (!json)
		DIE("cjson failed on file '%s'\n", path);
	
	if (isOne)
	{
		fprintf(gOut, "struct syOgmoRoom TODO = {\n");
		fprintf(gOut, " .filename = \"%s\",\n", doRoomPath(path));
		PrintJSON(json->child);
		fprintf(gOut, "};\n");
	}
	else
	{
		fprintf(gOut, "{\n");
		fprintf(gOut, " .filename = \"%s\",\n", doRoomPath(path));
		PrintJSON(json->child);
		fprintf(gOut, "},\n");
	}
	
	cJSON_Delete(json);
	free(contents);
	return 0;
}

static int ftw_callback(const char *fpath, const struct stat *sb, int typeflag)
{
	if (S_ISREG(sb->st_mode))
		handleFile(fpath, false);
	
	(void)typeflag;
	
	return 0;
}

static void stringArrayAppend(char ***list, int *count, char *v)
{
	*list = realloc(*list, (*count + 1) * sizeof(*list));
	assert(*list);
	
	(*list)[*count] = v;
	*count += 1;
}

static void stringArrayPrint(char **list)
{
	int i;
	
	for (i = 0; list[i]; ++i)
		fprintf(stderr, "%s\n", list[i]);
}

static int handleDirectory(const char *path)
{
	if (ftw(path, ftw_callback, 0))
	{
		fprintf(stderr, "ftw error\n");
		return -1;
	}
	
	return 0;
}

static char *getJsonString(cJSON *v)
{
	char *r = sanitizeNewlines(v->valuestring);
	
	assert(r);
	
	return r;
}

static char *getJsonStringFromBool(cJSON *v)
{
	char *r = myStrdup(cJSON_IsTrue(v) ? "true" : "false");
	
	assert(r);
	
	return r;
}

static char *getJsonStringFromNumber(cJSON *v)
{
	int rSz = 256;
	char *r = malloc(rSz);
	
	assert(r);
	
	if (fmod(fabs(v->valuedouble), 1) < 0.00001)
		snprintf(r, rSz, "%.lf", v->valuedouble);
	else
		snprintf(r, rSz, "%lf", v->valuedouble);
	
	return r;
}

static char **getJsonStringArray(cJSON *v)
{
	int count = 0;
	char **r = 0;
	cJSON *walk;
	
	for (walk = v->child; walk; walk = walk->next)
		stringArrayAppend(&r, &count, getJsonString(walk));
	
	/* 0 signals end of string array */
	stringArrayAppend(&r, &count, 0);
	
	return r;
}

static float getJsonFloat(cJSON *v)
{
	return v->valuedouble;
}

static int getJsonInt(cJSON *v)
{
	return v->valueint;
}

static int getJsonBool(cJSON *v)
{
	return cJSON_IsTrue(v);
}

static struct Vec2f getJsonVec2f(cJSON *v)
{
	struct Vec2f r = {0};
	cJSON *walk;
	
	for (walk = v->child; walk; walk = walk->next)
	{
		if (!strcmp(walk->string, "x"))
			r.x = getJsonFloat(walk);
		else if (!strcmp(walk->string, "y"))
			r.y = getJsonFloat(walk);
	}
	
	return r;
}

static struct Vec2i getJsonVec2i(cJSON *v)
{
	struct Vec2i r = {0};
	cJSON *walk;
	
	for (walk = v->child; walk; walk = walk->next)
	{
		if (!strcmp(walk->string, "x"))
			r.x = getJsonInt(walk);
		else if (!strcmp(walk->string, "y"))
			r.y = getJsonInt(walk);
	}
	
	//fprintf(stderr, "%s: %d %d\n", v->string, r.x, r.y);
	return r;
}

static struct OgmoEntityValueDef *getEntityValueArray(cJSON *v, int *count)
{
	struct OgmoEntityValueDef *r = 0;
	cJSON *walk;
	
	if (!v)
		return 0;
	
	for (walk = v->child; walk; walk = walk->next)
	{
		cJSON *now;
		struct OgmoEntityValueDef my = {0};
		
		for (now = walk->child; now; now = now->next)
		{
			FIRSTCASE("name")
				my.name = getJsonString(now);
			CASE("definition")
				my.definition = getJsonString(now);
			CASE("display")
				my.display = getJsonInt(now);
			CASE("defaults")
			{
				if (cJSON_IsString(now))
				{
					char buf[4096];
					
					assert(my.definition);
					
					if (!strcasecmp(my.definition, "color"))
						snprintf(buf, sizeof(buf), "0x%s", now->valuestring + 1);
					else
						snprintf(buf, sizeof(buf), "\"%s\"", sanitizeNewlines(now->valuestring));
					
					my.defaultsAsString = myStrdup(buf);
				}
				else if (cJSON_IsNumber(now))
					my.defaultsAsString = getJsonStringFromNumber(now);
				else if (cJSON_IsBool(now))
					my.defaultsAsString = getJsonStringFromBool(now);
				//fprintf(stderr, "%s\n", my.defaultsAsString);
			}
			CASE("choices")
			{
				int i;
				
				my.u.Enum.choices = getJsonStringArray(now);
				
				for (i = 0; my.u.Enum.choices[i]; ++i)
					condenseWhitespace(my.u.Enum.choices[i], 0);
			}
			/*UNKNOWN
			{
				fprintf(stderr, "unknown entity value component '%s'\n", now->string);
				exit(EXIT_FAILURE);
			}*/
		}
		
		/* append */
		r = realloc(r, (*count + 1) * sizeof(*r));
		assert(r);
		r[*count] = my;
		*count += 1;
	}
	
	/* append unused onto the end */
	{
		struct OgmoEntityValueDef my = {0};
		my.name = UNUSED___;
		my.definition = "Boolean";
		my.defaultsAsString = "0";
		r = realloc(r, (*count + 1) * sizeof(*r));
		assert(r);
		r[*count] = my;
		*count += 1;
	}
	
	/* go ahead and convert definition to definition_c here */
	for (int i = 0; i < *count; ++i)
	{
		struct OgmoEntityValueDef *my = &r[i];
		SWITCH_STR_BEGIN(my->definition)
			SWITCH_STR_FIRSTCASE("Boolean")
				my->definition_c = "bool ";
			SWITCH_STR_CASE("Color")
				my->definition_c = "unsigned int ";
			SWITCH_STR_CASE("Enum")
				my->definition_c = "enum "; /* XXX fully populated in getEntityArray */
			SWITCH_STR_CASE("Filepath")
				my->definition_c = "const char *";
			SWITCH_STR_CASE("Float")
				my->definition_c = "float ";
			SWITCH_STR_CASE("Integer")
				my->definition_c = "int ";
			SWITCH_STR_CASE("String")
				my->definition_c = "const char *";
			SWITCH_STR_CASE("Text")
				my->definition_c = "const char *";
			SWITCH_STR_UNKNOWN
				my->definition_c = "unknown ";
		SWITCH_STR_END
	}
	
	return r;
}

static struct OgmoEntityDef *getEntityArray(cJSON *v, int *count)
{
	struct OgmoEntityDef *r = 0;
	cJSON *walk;
	
	if (!v)
		return 0;
	
	for (walk = v->child; walk; walk = walk->next)
	{
		cJSON *now;
		struct OgmoEntityDef my = {0};
		
		my.valuesProgrammer = myStrdup("");
		assert(my.valuesProgrammer);
		
		for (now = walk->child; now; now = now->next)
		{
			FIRSTCASE("exportID")
				my.exportID = getJsonString(now);
			CASE("name")
				my.name = getJsonString(now);
			CASE("limit")
				my.limit = getJsonInt(now);
			CASE("size")
				my.size = getJsonVec2f(now);
			CASE("origin")
				my.origin = getJsonVec2f(now);
			CASE("originAnchored")
				my.originAnchored = getJsonBool(now);
			CASE("shape")
				/* TODO */;
			CASE("color")
				my.color = getJsonString(now);
			CASE("tileX")
				my.tileX = getJsonBool(now);
			CASE("tileY")
				my.tileY = getJsonBool(now);
			CASE("tileSize")
				my.tileSize = getJsonVec2f(now);
			CASE("resizeableX")
				my.resizeableX = getJsonBool(now);
			CASE("resizeableY")
				my.resizeableY = getJsonBool(now);
			CASE("rotatable")
				my.rotatable = getJsonBool(now);
			CASE("rotationDegrees")
				my.rotationDegrees = getJsonFloat(now);
			CASE("canFlipX")
				my.canFlipX = getJsonBool(now);
			CASE("canFlipY")
				my.canFlipY = getJsonBool(now);
			CASE("canSetColor")
				my.canSetColor = getJsonBool(now);
			CASE("hasNodes")
				my.hasNodes = getJsonBool(now);
			CASE("nodeLimit")
				my.nodeLimit = getJsonInt(now);
			CASE("nodeDisplay")
				my.nodeDisplay = getJsonInt(now);
			CASE("nodeGhost")
				my.nodeGhost = getJsonBool(now);
			CASE("tags")
				my.tags = getJsonStringArray(now);
			CASE("values")
				my.values = getEntityValueArray(now, &my.valuesCount);
			CASE("texture")
				my.texture = getJsonString(now);
			CASE("textureImage")
				/* XXX not used */;
			UNKNOWN
			{
				fprintf(stderr, "unknown entity component '%s'\n", now->string);
				exit(EXIT_FAILURE);
			}
		}
		
		/* handle enums */
		if (my.values && my.name)
		{
			struct OgmoEntityValueDef *v = my.values;
			int i;
			
			for (i = 0; i < my.valuesCount; ++i, ++v)
			{
				if (!strcmp(v->definition, "Enum"))
				{
					char buf[1024];
					
					snprintf(
						buf
						, sizeof(buf)
						, "enum %s%s%s "
						, gEntityNamePrefix
						, my.name
						, v->name
					);
					v->definition_c = myStrdup(buf);
				}
			}
		}
		
		/* append */
		r = realloc(r, (*count + 1) * sizeof(*r));
		assert(r);
		r[*count] = my;
		*count += 1;
	}
	
	return r;
}

static struct OgmoProject *OgmoProjectLoad(const char *fn)
{
	struct OgmoProject *ogmo;
	char *contents = loadfilestring(fn);
	cJSON *head;
	cJSON *now;
	
	if (!contents)
	{
		fprintf(stderr, "failed to load file '%s'\n", fn);
		return 0;
	}
	
	head = cJSON_Parse(contents);
	if (!head)
	{
		fprintf(stderr, "cjson failed on file '%s'\n", fn);
		return 0;
	}
	
	ogmo = calloc(1, sizeof(*ogmo));
	assert(ogmo);
	
	{
		ogmo->relpath = myStrdup(fn);
		char *s;
		char *s1;
		s = strrchr(ogmo->relpath, '/');
		s1 = strrchr(ogmo->relpath, '\\');
		s = s > s1 ? s : s1;
		
		if (!s)
		{
			fprintf(stderr, "expected path in '%s'\n", fn);
			exit(EXIT_FAILURE);
		}
		
		*s = '\0';
	}
	
	for (now = head->child; now; now = now->next)
	{
		FIRSTCASE("name")
			ogmo->name = getJsonString(now);
		CASE("ogmoVersion")
			ogmo->ogmoVersion = getJsonString(now);
		CASE("levelPaths")
			ogmo->levelPaths = getJsonStringArray(now);
		CASE("backgroundColor")
			ogmo->backgroundColor = getJsonString(now);
		CASE("gridColor")
			ogmo->gridColor = getJsonString(now);
		CASE("anglesRadians")
			ogmo->anglesRadians = getJsonBool(now);
		CASE("directoryDepth")
			ogmo->directoryDepth = getJsonInt(now);
		CASE("layerGridDefaultSize")
			ogmo->layerGridDefaultSize = getJsonVec2i(now);
		CASE("levelDefaultSize")
			ogmo->levelDefaultSize = getJsonVec2i(now);
		CASE("levelMinSize")
			ogmo->levelMinSize = getJsonVec2i(now);
		CASE("levelMaxSize")
			ogmo->levelMaxSize = getJsonVec2i(now);
		CASE("levelValues")
			ogmo->levelValues = getJsonStringArray(now);
		CASE("defaultExportMode")
			ogmo->defaultExportMode = getJsonString(now);
		CASE("compactExport")
			ogmo->compactExport = getJsonBool(now);
		CASE("externalScript")
			ogmo->externalScript = getJsonString(now);
		CASE("playCommand")
			ogmo->playCommand = getJsonString(now);
		CASE("entityTags")
			ogmo->entityTags = getJsonStringArray(now);
		CASE("layers")
		{
			/* TODO */
		}
		CASE("entities")
			ogmo->entities = getEntityArray(now, &ogmo->entitiesCount);
		CASE("tilesets")
		{
			/* TODO */
		}
		UNKNOWN
		{
			fprintf(stderr, "'%s' unknown type '%s'\n", fn, now->string);
			return 0;
		}
	}
	
	cJSON_Delete(head);
	free(contents);
	
	return ogmo;
}

char *convertEscapeSequences(const char *src)
{
	char *dst = myStrdup(src);
	char *tok;
	
	assert(dst);
	
	/* convert tab characters */
	while ((tok = strstr(dst, "\\t")))
	{
		memmove(tok + 1, tok + 2, strlen(tok + 2) + 1);
		*tok = '\t';
	}
	
	return dst;
}

static void writeAll(const char *outfn, struct OgmoProject *ogmo)
{
	struct OgmoEntityDef *entity;
	
	assert(outfn);
	assert(ogmo);
	
	gOut_redirect(outfn);
	
	for (entity = ogmo->entities; entity < ogmo->entities + ogmo->entitiesCount; ++entity)
		fprintf(gOut, "#include " INCLUDE_LEFT "%s%s.h" INCLUDE_RIGHT"\n", gEntityNamePrefix, entity->name);
	
	gOut_redirect(0);
}

static void writeEnums(const char *outfn, struct OgmoProject *ogmo)
{
	struct OgmoEntityDef *entity;
	
	assert(outfn);
	assert(ogmo);
	
	gOut_redirect(outfn);
	
	fprintf(gOut, "enum\n{\n");
	
	for (entity = ogmo->entities; entity < ogmo->entities + ogmo->entitiesCount; ++entity)
	{
		fprintf(gOut, "%s%s%s", gTabStyle, gEntityNamePrefix, entity->name);
		if (entity == ogmo->entities)
			fprintf(gOut, " = 1");
		fprintf(gOut, ",\n");
	}
	
	fprintf(gOut, "};\n");
	
	gOut_redirect(0);
}

static void writeClasses(const char *outfn, struct OgmoProject *ogmo)
{
	struct OgmoEntityDef *entity;
	
	assert(outfn);
	assert(ogmo);
	
	gOut_redirect(outfn);
	
	fprintf(
		gOut
		, "/* a simple array of classes; you are\n"
		" * expected to wrap it yourself like so:\n"
		" * \n"
		" * const struct syOgmoEntityClass *anyNameYouLike[] =\n"
		" * {\n"
		" *    #include <thisFile.h>\n"
		" * };\n"
		" */\n\n"
	);
	
	for (entity = ogmo->entities; entity < ogmo->entities + ogmo->entitiesCount; ++entity)
		fprintf(gOut, "[%s%s] = &%s%sClass,\n", gEntityNamePrefix, entity->name, gEntityNamePrefix, entity->name);
	
	gOut_redirect(0);
}

static void writeRooms(const char *outfn, struct OgmoProject *ogmo)
{
	char where[4096];
	
	assert(outfn);
	assert(ogmo);
	
	gOut_redirect(outfn);
	
	fprintf(
		gOut
		, "/* a simple array of rooms; you are\n"
		" * expected to wrap it yourself like so:\n"
		" * \n"
		" * static struct syOgmoRoom anyNameYouLike[] =\n"
		" * {\n"
		" *    #include <thisFile.h>\n"
		" * };\n"
		" */\n\n"
	);
	
	snprintf(where, 4096, "%s/room", ogmo->relpath);
	handleDirectory(where);
	
	gOut_redirect(0);
}

static void writeSources(const char *outdir, struct OgmoProject *ogmo)
{
	struct OgmoEntityDef *entity;
	
	assert(outdir);
	assert(ogmo);
	
	for (entity = ogmo->entities; entity < ogmo->entities + ogmo->entitiesCount; ++entity)
	{
		char *file;
		char outpath[4096];
		char name[1024];
		static char *heap = 0;
		int heapSz = 8 MIB;
		bool fileHasChanged = false;
		
		if (!heap)
		{
			heap = malloc(heapSz);
			assert(heap);
		}
		
		snprintf(name, sizeof(name), "%s%s", gEntityNamePrefix, entity->name);
		
		snprintf(outpath, sizeof(outpath), "%s/%s.c", outdir, name);
		
		fprintf(stderr, "processing file '%s'\n", outpath);
		
		file = loadfilestringFast(outpath, 0);
		
		if (!file)
		{
			size_t sz;
			fprintf(stderr, "file '%s' doesn't exist, creating it...\n", outpath);
			file = loadfilestringFast("", &sz);
			fileHasChanged = true;
			
			/* populate with default contents */
			snprintf(file, sz,
				"/* \n"
				" * %s.c <ProjectName>\n"
				" * \n"
				" * your description here\n"
				" * \n"
				" */\n"
				"\n"
				"#include <%s.h>\n"
				"\n"
				"/* <ogmodefaults> */\n"
				"const struct %s %s""Defaults = \n"
				"{\n"
				"%s""/* <ogmoblock> */\n"
				"%s""/* </ogmoblock> */\n"
				"%s""\n"
				"%s""/* <ogmoblock1> */\n"
				"%s""/* defaults for custom user-defined variables go here */\n"
				"%s""/* </ogmoblock1> */\n"
				"};\n"
				"/* </ogmodefaults> */\n"
				"\n"
				"/* <ogmonew> */\n"
				"static void *New(const void *src)\n"
				"{\n"
				"%s""/* <ogmoblock> */\n"
				"%s""/* </ogmoblock> */\n"
				"%s""\n"
				"%s""/* custom code goes here */\n"
				"%s""\n"
				"%s""/* <ogmoblock1> */\n"
				"%s""/* </ogmoblock1> */\n"
				"}\n"
				"/* </ogmonew> */\n"
				"\n"
				"syOgmoEntityFuncDecl(Draw)\n"
				"{\n"
				"%s""#include \"debug_draw.h\"\n"
				"\n"
				"%s""return 0;\n"
				"}\n"
				"\n"
				"/* <ogmoclass> */\n"
				"/* </ogmoclass> */\n"
				"\n"
				, name /* En_Test.c */
				, name /* En_Test.h */
				, name /* struct En_Test */
				, name /* En_TestDefaults */
				, gTabStyle /* within <ogmodefaults> */
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle /* within <ogmonew> */
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, gTabStyle /* within Draw */
				, gTabStyle
			);
		}
		
		{
			struct StringRegion r;
			
			/* XXX work from end to beginning within the region */
			r = OgmoRegion(file, "ogmodefaults");
			{
				struct StringRegion sr;
				
				/* ogmoblock1 contains some initializers, so retrieve and sanitize */
				sr = OgmoRegionInRegion(r, "ogmoblock1");
				{
					char *tmp = entity->valuesProgrammer;
					char *ss;
					
					assert(tmp);
					free(tmp);
					
					tmp = malloc(sr.length + 10);
					assert(tmp);
					memcpy(tmp, sr.begin, sr.length);
					tmp[sr.length] = '\0';
					
					/* remove comments */
					removeComments(tmp);
					
					/* remove characters that aren't valid in struct initializers */
					for (ss = tmp; *ss; ++ss)
					{
						if (*ss == '"')
							ss = skipQuote(ss);
						else if (!isalnum(*ss) && !strchr(".,_=\"'", *ss))
							*ss = ' ';
					}
					
					/* guarantee doesn't begin with ',' */
					for (ss = tmp; *ss && !isalnum(*ss); ++ss)
						if (*ss == ',')
							*ss = ' ';
					
					/* condense whitespace */
					condenseWhitespace(tmp, 1);
					
					/* guarantee it ends with ',' */
					for (ss = tmp + strlen(tmp) - 1; ss > tmp; --ss)
						if (isalnum(*ss) || strchr("\"'", *ss))
							break;
					if (!strchr(ss, ','))
						strcat(ss, ",");
					
					/* if no initializers are present, use blank string */
					if (!strchr(tmp, '.'))
						strcpy(tmp, "");
					
					entity->valuesProgrammer = tmp;
					
					//fprintf(stderr, "tmp = '%s'\n", tmp);
				}
				
				/* ogmoblock is where the main initializers live */
				sr = OgmoRegionInRegion(r, "ogmoblock");
				{
					/* handle unset defaults */
					struct OgmoEntityValueDef *v;
					char buf1[16];
					char buf2[16];
					const char *prefix;
					
					strcpy(buf1, "\n");
					strcat(buf1, gTabStyle);
					
					strcpy(buf2, gTabStyle);
					strcat(buf2, ", ");
					
					prefix = buf1;
					
					strcpy(heap, "");
					for (v = entity->values; v < entity->values + entity->valuesCount; ++v)
					{
						strcatf(heap, "%s.%s = %s\n", prefix, v->name, v->defaultsAsString);
						prefix = buf2;
					}
					strcatf(heap, gTabStyle);
					//fprintf(stdout, "%s\n", heap);
					
					fileHasChanged |= OgmoRegionReplace(sr, heap);
				}
			}
			
			/* XXX work from end to beginning within the region */
			r = OgmoRegion(file, "ogmonew");
			{
				struct StringRegion sr;
				
				/* ogmoblock1 contains a simple return from New() function */
				sr = OgmoRegionInRegion(r, "ogmoblock1");
				{
					sprintf(heap, "\n%sreturn my;\n%s", gTabStyle, gTabStyle);
					fileHasChanged |= OgmoRegionReplace(sr, heap);
				}
				
				/* ogmoblock contains a generic allocation routine */
				sr = OgmoRegionInRegion(r, "ogmoblock");
				{
					sprintf(heap,
						"\n"
						"%s""struct %s *my = MALLOC(sizeof(*my));\n"
						"%s""\n"
						"%s""assert(my);\n"
						"%s""\n"
						"%s""if (!src)\n"
						"%s%s""src = &%sDefaults;\n"
						"%s""\n"
						"%s""memcpy(my, src, sizeof(*my));\n%s"
						, gTabStyle, name
						, gTabStyle
						, gTabStyle
						, gTabStyle
						, gTabStyle
						, gTabStyle
						, gTabStyle, name
						, gTabStyle
						, gTabStyle, gTabStyle
					);
					fileHasChanged |= OgmoRegionReplace(sr, heap);
				}
			}
			
			/* XXX work from end to beginning within the region */
			r = OgmoRegion(file, "ogmoclass");
			{
				char *f2;
				char *ss;
				const char *parentClass = strstr(r.begin, "parentClass");
				
				if (parentClass)
				{
					parentClass = strchr(parentClass, '=');
					if (parentClass)
						while (*parentClass && !isalnum(*parentClass) && *parentClass != '_')
							++parentClass;
				}
				
				sprintf(heap, "\n""const struct syOgmoEntityClass %sClass = {\n"
					"%s"".New = New\n"
					"%s"", .funcs = (syOgmoEntityFunc[]){\n"
					, name
					, gTabStyle /* New */
					, gTabStyle /* funcs */
				);
				
				/* it is assumed that there is room for an extra copy at the end */
				f2 = file + strlen(file) + 1;
				strcpy(f2, file);
				
				removeComments(f2);
				
				/* TODO the following two blocks have a lot
				 * of repeated code; simplify it sometime
				 */
				
				/* print function array */
				{
					const char *prefix = "";
					
					for (ss = f2; ; )
					{
						int r;
						
						ss = strstr(ss, "syOgmoEntityFuncDecl");
						
						if (!ss)
							break;
						
						ss = strchr(ss, '(');
						if (!ss)
							break;
						
						while (*ss && !isalpha(*ss))
							++ss;
						if (!*ss)
							break;
						
						r = strcspn(ss, ")\r\n\t ");
						strcatf(
							heap
							, "%s%s%s""[syOgmoExec_%.*s] = %.*s\n"
							, gTabStyle
							, gTabStyle
							, prefix
							, r, ss
							, r, ss
						);
						prefix = ", ";
					}
					strcatf(heap, "%s""}\n", gTabStyle);
				}
				
				/* and then size of function array */
				{
					const char *prefix = "";
					
					strcatf(heap,
						"%s"", .funcsCount = sizeof((char[]){\n"
						, gTabStyle /* funcsCount */
					);
					
					for (ss = f2; ; )
					{
						int r;
						
						ss = strstr(ss, "syOgmoEntityFuncDecl");
						
						if (!ss)
							break;
						
						ss = strchr(ss, '(');
						if (!ss)
							break;
						
						while (*ss && !isalpha(*ss))
							++ss;
						if (!*ss)
							break;
						
						r = strcspn(ss, ")\r\n\t ");
						strcatf(
							heap
							, "%s%s%s""[syOgmoExec_%.*s] = 0\n"
							, gTabStyle
							, gTabStyle
							, prefix
							, r, ss
						);
						prefix = ", ";
					}
					strcatf(heap, "%s""}) / sizeof(char)\n", gTabStyle);
				}
				if (parentClass)
					strcatf(
						heap
						, "%s, .parentClass = %.*s\n"
						, gTabStyle
						, strcspn(parentClass, " \r\n\t")
						, parentClass
					);
				
				strcatf(heap,
					"};\n"
					, gTabStyle
				);
				
				fileHasChanged |= OgmoRegionReplace(r, heap);
			}
			
			
			//fprintf(stderr, "%.*s\n", r.length, r.begin);
		}
		
		if (fileHasChanged)
			writefilestring(outpath, file);
		else
			fprintf(stderr, " -> no changes required, not overwriting\n");
		//fwrite(file, 1, strlen(file) + 1, stderr);
		
		/* loadfilestringFast() returns a heap that is reused
		 * across invocations, so don't free it
		 */
		//free(file);
	}
}

static void writeHeaders(const char *outdir, struct OgmoProject *ogmo)
{
	struct OgmoEntityDef *entity;
	
	assert(outdir);
	assert(ogmo);
	
	for (entity = ogmo->entities; entity < ogmo->entities + ogmo->entitiesCount; ++entity)
	{
		char *file;
		char outpath[4096];
		char name[1024];
		static char *heap = 0;
		int heapSz = 8 MIB;
		bool fileHasChanged = false;
		
		if (!heap)
		{
			heap = malloc(heapSz);
			assert(heap);
		}
		
		snprintf(name, sizeof(name), "%s%s", gEntityNamePrefix, entity->name);
		
		snprintf(outpath, sizeof(outpath), "%s/%s.h", outdir, name);
		
		fprintf(stderr, "processing file '%s'\n", outpath);
		
		file = loadfilestringFast(outpath, 0);
		
		if (!file)
		{
			size_t sz;
			fprintf(stderr, "file '%s' doesn't exist, creating it...\n", outpath);
			file = loadfilestringFast("", &sz);
			fileHasChanged = true;
			
			/* populate with default contents */
			snprintf(file, sz,
				"/* \n"
				" * %s.h <ProjectName>\n"
				" * \n"
				" * your description here\n"
				" * \n"
				" */\n"
				"\n"
				"#ifndef %s_H_INCLUDED\n"
				"#define %s_H_INCLUDED\n"
				"\n"
				"#include %s\n"
				"\n"
				"/* <ogmoenums> */\n"
				"/* </ogmoenums> */\n"
				"\n"
				"/* <ogmostruct> */\n"
				"struct %s\n"
				"{\n"
				"%s""/* <ogmoblock> */\n"
				"%s""/* </ogmoblock> */\n"
				"%s""\n"
				"%s""/* custom user-defined variables go here */\n"
				"};\n"
				"/* <ogmoblock1> */\n"
				"/* </ogmoblock1> */\n"
				"/* </ogmostruct> */\n"
				"\n"
				"#endif /* %s_H_INCLUDED */\n"
				"\n"
				, name /* En_Test.h */
				, name /* include guard */
				, name /* also include guard */
				, "\"common-includes.h\""
				, name /* struct En_Test */
				, gTabStyle /* within <ogmostruct> */
				, gTabStyle
				, gTabStyle
				, gTabStyle
				, name /* include guard comment */
			);
		}
		
		{
			struct StringRegion r;
			
			r = OgmoRegion(file, "ogmoenums");
			{
				struct OgmoEntityValueDef *v;
				
				strcpy(heap, "\n");
				for (v = entity->values; v < entity->values + entity->valuesCount; ++v)
				{
					char buf[512];
					const char *prefix = "";
					char **choices = v->u.Enum.choices;
					int i;
					
					/* skip non-enum types */
					if (!v->definition)
						continue;
					if (strcasecmp(v->definition, "enum"))
						continue;
					
					snprintf(buf, sizeof(buf), "%s%s", name, v->name);
					
					strcatf(heap, "enum %s\n{\n", buf);
					for (i = 0; choices[i]; ++i)
					{
						strcatf(heap, "%s%s%s_%s\n", gTabStyle, prefix, buf, choices[i]);
						prefix = ", ";
					}
					strcatf(heap, "%s%s%s_%s\n", gTabStyle, prefix, buf, "COUNT");
					strcatf(heap, "};\n");
				}
				
				fileHasChanged |= OgmoRegionReplace(r, heap);
			}
			
			r = OgmoRegion(file, "ogmoblock");
			{
				struct OgmoEntityValueDef *v;
				
				strcpy(heap, "\n");
				for (v = entity->values; v < entity->values + entity->valuesCount; ++v)
					strcatf(heap, "%s%s%s;\n", gTabStyle, v->definition_c, v->name);
				strcatf(heap, "%s", gTabStyle);
				
				fileHasChanged |= OgmoRegionReplace(r, heap);
			}
			
			r = OgmoRegion(file, "ogmoblock1");
			{
				struct OgmoEntityValueDef *v;
				
				strcpy(heap, "\n");
				strcatf(heap,
					"extern const struct %s %s""Defaults;\n"
					"extern const struct syOgmoEntityClass %s""Class;\n"
					"#define %s""Values(...) (struct %s){"
					, name, name, name, name, name
				);
				for (v = entity->values; v < entity->values + entity->valuesCount; ++v)
					strcatf(heap, ".%s = %s, ", v->name, v->defaultsAsString);
				strcatf(heap, "%s __VA_ARGS__ }\n", entity->valuesProgrammer);
				
				fileHasChanged |= OgmoRegionReplace(r, heap);
			}
		}
		
		/* TODO  */
		/*{
		}*/
		
		if (fileHasChanged)
			writefilestring(outpath, file);
		else
			fprintf(stderr, " -> no changes required, not overwriting\n");
		//fwrite(file, 1, strlen(file) + 1, stderr);
		
		/* loadfilestringFast() returns a heap that is reused
		 * across invocations, so don't free it
		 */
		//free(file);
	}
}

int main(int argc, char *argv[])
{
	struct OgmoProject *ogmo;
	const char *in = argv[1];
	const char *tabStyle = argv[2];
	const char *prefix = argv[3];
	const char *src = argv[4];
	const char *include = argv[5];
	const char *all = argv[6];
	const char *enums = argv[7];
	const char *classes = argv[8];
	const char *rooms = argv[9];
	
	gOut = stdout;
	
	if (argc != 10)
	{
		fprintf(stderr, "args: ogmo2c in tab prefix src include all enums classes rooms\n");
		fprintf(stderr, " - in:       the path to the .ogmo file,\n");
		fprintf(stderr, " - tab:      tab style \"\\t\" or \"   \" etc\n");
		fprintf(stderr, " - prefix:   prefix to prepend to entity struct/class names\n");
		fprintf(stderr, " - src:      directory in which entity sources reside\n");
		fprintf(stderr, " - include:  directory in which entity headers reside\n");
		fprintf(stderr, " - all:      file in which to write an #include for each entity\n");
		fprintf(stderr, " - enums:    file in which entity enums reside\n");
		fprintf(stderr, " - classes:  file in which entity class pointers reside\n");
		fprintf(stderr, " - rooms:    file in which room database reside\n");
		
		return -1;
	}
	
	gTabStyle = convertEscapeSequences(tabStyle);
	gEntityNamePrefix = prefix;
	
	
	ogmo = OgmoProjectLoad(in);
	gOgmoProject = ogmo;
	fprintf(stderr, "%p\n", ogmo);
	if (!ogmo)
		return -1;
	
	stringArrayPrint(ogmo->levelPaths);
	
	/* XXX order matters; in particular, writeSources() should precede
	 * everything else because defaults for programmer-defined variables
	 * are retrieved that way and used elsewhere!
	 */
	writeSources(src, ogmo);
	
	writeHeaders(include, ogmo);
	
	writeAll(all, ogmo);
	
	writeEnums(enums, ogmo);
	
	writeClasses(classes, ogmo);
	
	writeRooms(rooms, ogmo);
	
	return 0;
	
	{
		char *path = "";
		if (isDirectory(path))
			return handleDirectory(path);
		else if (isFile(path))
			return handleFile(path, true);
		
		fprintf(stderr, "failed to process '%s'; does it exist?\n"
			"is it a file? is it a directory?\n"
			, path
		);
		return -1;
		(void)in;
		(void)include;
		(void)src;
	}
}

