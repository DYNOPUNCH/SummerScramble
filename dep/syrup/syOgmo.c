/*
 * syOgmo.h <SyrupEngine>
 *
 * ogmo game room types and functions
 *
 */

#include "syOgmo.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

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

#define DUP_ROSTRING(X) if (dst->X) dst->X = myStrdup(dst->X)

/*
 *
 * private functions
 *
 */

static struct syOgmoEntity *CloneEntity(const struct syOgmoEntity *src, bool readonlyToo);

static void myError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	fflush(stderr); /* msys2 */
	va_end(args);
	exit(EXIT_FAILURE);
}

/* private global room context */
static struct
{
	struct Game *game;
	const struct syOgmoRoom *db;
	int dbLen;
	void (*error)(const char *fmt, ...);
	void *(*image_load)(struct Game *game, const char *fn);
	void (*image_draw)(struct Game *game, void *img, float ulx, float uly);
	void (*image_free)(struct Game *game, void *img);
	int (*image_get_width)(struct Game *game, void *img);
	int (*image_get_height)(struct Game *game, void *img);
	const struct syOgmoEntityClass **entityClassDb;
	int entityClassDbLen;
	syOgmoExec funcKeyPrev;
} g = {
	.error = myError
};

/* private types */
struct syRoom
{
	struct syOgmoRoom ogmo;
};

static const struct syOgmoEntityClass *GetClass(syOgmoEntityClass class)
{
	assert(g.entityClassDb);
	assert(class < g.entityClassDbLen);
	
	return g.entityClassDb[class];
}

static struct syOgmoEntity *CloneEntityInto(struct syOgmoEntity *dst, const struct syOgmoEntity *src, bool readonlyToo)
{
	assert(dst);
	assert(src);
	
	memcpy(dst, src, sizeof(*src));
	
	if (readonlyToo)
	{
		DUP_ROSTRING(name);
		DUP_ROSTRING(_eid);
	}
	
	if (dst->values)
	{
		const struct syOgmoEntityClass *class = GetClass(dst->valuesClass);
		
		assert(class->New);
		dst->values = class->New(dst->values);
	}
	
	if (dst->parent)
		dst->parent = CloneEntity(dst->parent, readonlyToo);
	else if (dst->parentClass)
		dst->parent = syOgmoEntityNew(dst->parentClass);
	
	return dst;
}

static struct syOgmoEntity *CloneEntity(const struct syOgmoEntity *src, bool readonlyToo)
{
	struct syOgmoEntity *dst = calloc(1, sizeof(*dst));
	
	assert(dst);
	
	return CloneEntityInto(dst, src, readonlyToo);
}

static struct syOgmoEntity *CloneEntities(const struct syOgmoEntity *src, int count, bool readonlyToo)
{
	struct syOgmoEntity *head;
	struct syOgmoEntity *dst;
	
	if (!src)
		return 0;
	
	head = dst = malloc(count * sizeof(*src));
	assert(head);
	assert(dst);
	
	for (dst = head; dst < head + count; ++dst, ++src)
		CloneEntityInto(dst, src, readonlyToo);
	
	return head;
}

static struct syOgmoDecal *CloneDecals(const struct syOgmoDecal *src, int count, bool readonlyToo)
{
	struct syOgmoDecal *head;
	struct syOgmoDecal *dst;
	
	if (!src)
		return 0;
	
	head = dst = malloc(count * sizeof(*src));
	assert(head);
	assert(dst);
	
	memcpy(dst, src, count * sizeof(*src));
	
	for (dst = head; dst < head + count; ++dst)
	{
		if (readonlyToo)
		{
			DUP_ROSTRING(texture);
		}
	}
	
	return head;
}

static struct syOgmoLayer *CloneLayers(const struct syOgmoLayer *src, int count, bool readonlyToo)
{
	struct syOgmoLayer *head;
	struct syOgmoLayer *dst;
	
	if (!src)
		return 0;
	
	head = dst = malloc(count * sizeof(*src));
	assert(head);
	assert(dst);
	
	memcpy(dst, src, count * sizeof(*src));
	
	for (dst = head; dst < head + count; ++dst)
	{
		if (readonlyToo)
		{
			DUP_ROSTRING(name);
			DUP_ROSTRING(_eid);
			DUP_ROSTRING(folder);
		}
		
		dst->entities = CloneEntities(dst->entities, dst->entitiesCount, readonlyToo);
		dst->decals = CloneDecals(dst->decals, dst->decalsCount, readonlyToo);
	}
	
	return head;
}

static syOgmoExecRetval ExecEntity(struct syOgmoEntity *entity, syOgmoExec funcKey)
{
	const struct syOgmoEntityClass *class;
	const syOgmoEntityFunc *funcs;
	syOgmoEntityFunc exec = 0;
	
	assert(entity);
	g.funcKeyPrev = funcKey;
	
	class = GetClass(entity->valuesClass);
	funcs = class->funcs;
	
	assert(funcs);
	
	/* no function indicated for this event, so inherit from parent (if applicable) */
	if (funcKey >= class->funcsCount
		|| !(exec = funcs[funcKey])
	)
		return syOgmoEntityInheritEvent(entity);
	
	return exec(g.game, entity);
}

static void ExecEntityArray(struct syOgmoEntity *array, int count, syOgmoExec funcKey)
{
	struct syOgmoEntity *entity = array;
	int i;
	
	if (!entity)
		return;
	
	for (i = 0; i < count; ++i, ++entity)
		ExecEntity(entity, funcKey);
}

static void ExecEntityList(struct syOgmoEntity *list, syOgmoExec funcKey)
{
	struct syOgmoEntity *entity;
	
	if (!list)
		return;
	
	for (entity = list; entity; entity = entity->next)
		ExecEntity(entity, funcKey);
}

static void DrawDecal(struct syOgmoDecal *decal)
{
	assert(decal);
	assert(g.image_draw);
	assert(g.image_get_width);
	assert(g.image_get_height);
	
	void *udata = decal->udata;
	
	g.image_draw(
		g.game
		, udata
		, decal->x - decal->originX * g.image_get_width(g.game, udata)
		, decal->y - decal->originY * g.image_get_height(g.game, udata)
	);
}

static void DrawDecalArray(struct syOgmoDecal *array, int count)
{
	struct syOgmoDecal *decal = array;
	int i;
	
	if (!decal)
		return;
	
	for (i = 0; i < count; ++i, ++decal)
		DrawDecal(decal);
}

static void LoadDecal(struct syOgmoDecal *decal, const char *folder)
{
	char tmp[1024];
	
	assert(decal);
	assert(g.image_load);
	
	snprintf(tmp, sizeof(tmp), "%s/%s", folder, decal->texture);
	
	decal->udata = g.image_load(g.game, tmp);
}

static void LoadDecalArray(struct syOgmoDecal *array, int count, const char *folder)
{
	struct syOgmoDecal *decal = array;
	int i;
	
	if (!decal)
		return;
	
	for (i = 0; i < count; ++i, ++decal)
		LoadDecal(decal, folder);
}

static void FreeDecal(struct syOgmoDecal *decal)
{
	assert(decal);
	assert(g.image_free);
	
	g.image_free(g.game, decal->udata);
}

static void FreeDecalArray(struct syOgmoDecal *array, int count)
{
	struct syOgmoDecal *decal = array;
	int i;
	
	if (!decal)
		return;
	
	for (i = 0; i < count; ++i, ++decal)
		FreeDecal(decal);
}

static const char *UseOgmoPath(const char *str)
{
	static char ogmoFolder[1024];
	
	snprintf(ogmoFolder, sizeof(ogmoFolder), "ogmo/%s", str);
	
	return ogmoFolder;
}

static void LoadOgmoAssets(const struct syOgmoRoom *ogmo)
{
	struct syOgmoLayer *layer = ogmo->layers;
	int count = ogmo->layersCount;
	int i;
	
	if (!layer)
		return;
	
	for (i = 0; i < count; ++layer, ++i)
		LoadDecalArray(layer->decals, layer->decalsCount, UseOgmoPath(layer->folder));
}

/* makes a deep copy of a syOgmoRoom from src (source) into dst (destination)
 * when readonlyToo is true, readonly strings are also copied in memory
 * XXX udata is not (currently?) deep copied
 */
void syOgmoRoomCloneInto(struct syOgmoRoom *dst, const struct syOgmoRoom *src, bool readonlyToo)
{
	assert(dst);
	assert(src);
	
	memcpy(dst, src, sizeof(*src));
	
	if (readonlyToo)
	{
		DUP_ROSTRING(filename);
		DUP_ROSTRING(ogmoVersion);
	}
	
	dst->layers = CloneLayers(dst->layers, dst->layersCount, readonlyToo);
	dst->ownsRodata = readonlyToo;
	
	// TODO merge syOgmoRoom and syRoom types
	syRoomExec((void*)dst, syOgmoExec_Init);
}

void syRoomContextSetGame(struct Game *game)
{
	g.game = game;
}

void syRoomContextSetError(void error(const char *fmt, ...))
{
	g.error = error;
}

void syRoomContextSetOgmoDatabase(const struct syOgmoRoom *array, const int arrayNum)
{
	g.db = array;
	g.dbLen = arrayNum;
}

void syRoomContextSetEntityClassDatabase(const struct syOgmoEntityClass **array, const int arrayNum)
{
	g.entityClassDb = array;
	g.entityClassDbLen = arrayNum;
}

void syRoomContextSetImageLoad(void *image_load(struct Game *game, const char *fn))
{
	g.image_load = image_load;
}

void syRoomContextSetImageDraw(void image_draw(struct Game *game, void *img, float ulx, float uly))
{
	g.image_draw = image_draw;
}

void syRoomContextSetImageFree(void image_free(struct Game *game, void *img))
{
	g.image_free = image_free;
}

void syRoomContextSetImageGetWidth(int image_get_width(struct Game *game, void *img))
{
	g.image_get_width = image_get_width;
}

void syRoomContextSetImageGetHeight(int image_get_height(struct Game *game, void *img))
{
	g.image_get_height = image_get_height;
}

struct syRoom *syRoomNewFromOgmo(const struct syOgmoRoom *rd)
{
	struct syRoom *r = calloc(1, sizeof(*r));
	
	assert(r);
	
	syOgmoRoomCloneInto(&r->ogmo, rd, false);
	
	LoadOgmoAssets(&r->ogmo);
	
	return r;
}

struct syRoom *syRoomNewFromFilename(const char *filename)
{
	const struct syOgmoRoom *rd;
	
	for (rd = g.db; rd < g.db + g.dbLen; ++rd)
		if (rd->filename && !strcmp(rd->filename, filename))
			return syRoomNewFromOgmo(rd);
	
	g.error("room '%s' does not exist...", filename);
	return 0;
}

void syRoomDraw(struct syRoom *r)
{
	assert(r);
	
	struct syOgmoLayer *layer = r->ogmo.layers;
	int count = r->ogmo.layersCount;
	int i;
	
	if (!layer)
		return;
	
	/* draw layers in reverse order */
	layer += count - 1;
	for (i = 0; i < count; --layer, ++i)
	{
		ExecEntityArray(layer->entities, layer->entitiesCount, syOgmoExec_Draw);
		ExecEntityList(layer->spawnedEntities, syOgmoExec_Draw);
		DrawDecalArray(layer->decals, layer->decalsCount);
	}
}

void syRoomDelete(struct syRoom *r)
{
	/* TODO */
}

void syOgmoLayerAddInstance(struct syOgmoLayer *layer, struct syOgmoEntity *entity)
{
	assert(layer);
	assert(entity);
	
	entity->next = layer->spawnedEntities;
	layer->spawnedEntities = entity;
}

struct syOgmoEntity *syOgmoEntityNew(syOgmoEntityClass type)
{
	struct syOgmoEntity *entity = calloc(1, sizeof(*entity));
	const struct syOgmoEntityClass *class;
	
	assert(entity);
	
	memcpy(entity, &(struct syOgmoEntity){.valuesClass = type}, sizeof(*entity));
	
	class = GetClass(type);
	
	assert(class->New);
	entity->values = class->New(0);
	
	if (class->parentClass)
		entity->parent = syOgmoEntityNew(entity->parentClass);
	
	/* TODO perhaps move this into the wrapper instead */
	ExecEntity(entity, syOgmoExec_Init);
	
	return entity;
}

int syOgmoEntityInheritEvent(struct syOgmoEntity *entity)
{
	if (!entity)
		return 0;
	
	if (entity->parent)
		return ExecEntity(entity->parent, g.funcKeyPrev);
	
	return 0;
}

void syRoomExec(struct syRoom *r, syOgmoExec funcKey)
{
	assert(r);
	
	struct syOgmoLayer *layer = r->ogmo.layers;
	int count = r->ogmo.layersCount;
	int i;
	
	if (!layer)
		return;
	
	for (i = 0; i < count; ++layer, ++i)
	{
		ExecEntityArray(layer->entities, layer->entitiesCount, funcKey);
		ExecEntityList(layer->spawnedEntities, funcKey);
	}
}

