/*
 * syOgmo.h <SyrupEngine>
 *
 * ogmo game room types and functions
 *
 */

#ifndef SYRUP_OGMO_H_INCLUDED
#define SYRUP_OGMO_H_INCLUDED

#include <stdbool.h>

struct Game;
struct syOgmoEntity;
struct syOgmoDecal;
struct syOgmoLayer;
struct syOgmoRoom;
struct syOgmoEntityClass;
typedef int syOgmoEntityClass;
typedef int syOgmoExec;
typedef int syOgmoExecRetval;
typedef syOgmoExecRetval (*syOgmoEntityFunc)(struct Game *game, struct syOgmoEntity *entity);
#define syOgmoEntityFuncShareArgs game, ogmo
#define syOgmoEntityFuncDecl(FUNCNAME) static syOgmoExecRetval FUNCNAME(struct Game *game, struct syOgmoEntity *ogmo)
#define syOgmoEntityFuncPublic(FUNCNAME) syOgmoExecRetval FUNCNAME(struct Game *game, struct syOgmoEntity *ogmo)
#define syOgmoEntityFuncLast ((syOgmoEntityFunc)-1)


#define ___syOgmoEntitySpawnConcat(X, ...) X ## Values( __VA_ARGS__ )
#define ___syOgmoEntitySpawnMain(X, ...) (syOgmoEntityNew)(X, &___syOgmoEntitySpawnConcat(X, __VA_ARGS__))
#define ___syOgmoEntitySpawnMainWrapper(X) X
#define syOgmoEntityNewWith(...) ___syOgmoEntitySpawnMainWrapper(___syOgmoEntitySpawnMain(__VA_ARGS__, .unused___ = 0))

void syOgmoRoomCloneInto(struct syOgmoRoom *dst, const struct syOgmoRoom *src, bool readonlyToo);
int syOgmoLayerCount(const struct syOgmoLayer *array);
int syOgmoDecalCount(const struct syOgmoDecal *array);
int syOgmoEntityCount(const struct syOgmoEntity *array);
struct syOgmoEntity *syOgmoEntityNew(syOgmoEntityClass type, const void *values);
int syOgmoEntityInheritEvent(struct syOgmoEntity *entity);

/* public functions */

/* opaque types */
struct syRoom;
void syRoomContextSetGame(struct Game *game);
void syRoomContextSetError(void error(const char *fmt, ...));
void syRoomContextSetOgmoDatabase(const struct syOgmoRoom *array, const int arrayNum);
void syRoomContextSetEntityClassDatabase(const struct syOgmoEntityClass **array, const int arrayNum);
void syRoomContextSetImageLoad(void *image_load(struct Game *game, const char *fn));
void syRoomContextSetImageDraw(void image_draw(struct Game *game, void *img, float ulx, float uly));
void syRoomContextSetImageFree(void image_free(struct Game *game, void *img));
void syRoomContextSetImageGetWidth(int image_get_width(struct Game *game, void *img));
void syRoomContextSetImageGetHeight(int image_get_height(struct Game *game, void *img));

struct syRoom *syRoomNewFromOgmo(const struct syOgmoRoom *rd);
struct syRoom *syRoomNewFromFilename(const char *filename);
void syRoomExec(struct syRoom *r, int func);
void syRoomDraw(struct syRoom *r);
void syRoomDelete(struct syRoom *r);

enum syOgmoExecRetval
{
	syOgmoExecRetval_Success = 0
};

enum syOgmoExec
{
	syOgmoExec_Init = 0
	, syOgmoExec_Step
	, syOgmoExec_Draw
	, syOgmoExec_Free
	, syOgmoExec_End
	/* values >= syOgmoExec_End should be defined in your own game-specific enum
	 * (declared elsewhere), and follow the same naming convention
	 */
};

/* example of the above */
/*enum syOgmoExecExtended
{
	syOgmoExec_BeginStep = syOgmoExec_End
	, syOgmoExec_EndStep
	, syOgmoExec_PreDraw
	, syOgmoExec_PostDraw
};*/

struct syOgmoEntityClass
{
	void *(*New)(const void *src);
	const syOgmoEntityFunc *funcs;
	const int funcsCount;
	const syOgmoEntityClass parentClass;
};

struct syOgmoEntity
{
	const char *name;
	const char *_eid;
	void *values;
	void *udata; /* extra */
	struct syOgmoEntity *next; /* extra (linked list) */
	struct syOgmoEntity *parent; /* extra */
	const syOgmoEntityClass valuesClass; /* extra */
	const syOgmoEntityClass parentClass; /* extra */
	const int id;
	const float x;
	const float y;
	const int width;
	const int height;
	const float originX;
	const float originY;
	const float rotation;
	const bool flippedX;
	const bool flippedY;
};

struct syOgmoDecal
{
	const char *texture;
	void *udata; /* extra */
	const float x;
	const float y;
	const float originX;
	const float originY;
};

struct syOgmoLayer
{
	const char *name;
	const char *_eid;
	struct syOgmoEntity *entities;
	struct syOgmoDecal *decals;
	const char *folder;
	void *udata; /* extra */
	struct syOgmoEntity *spawnedEntities; /* extra (linked list of entities spawned side from map) */
	const int offsetX;
	const int offsetY;
	const int gridCellWidth;
	const int gridCellHeight;
	const int gridCellsX;
	const int gridCellsY;
	const int entitiesCount; /* extra */
	const int decalsCount; /* extra */
};

struct syOgmoRoom
{
	const char *filename;
	const char *ogmoVersion;
	struct syOgmoLayer *layers;
	void *udata; /* extra */
	const int width;
	const int height;
	const int offsetX;
	const int offsetY;
	const int layersCount; /* extra */
	bool ownsRodata; /* extra */
};

#endif /* SYRUP_OGMO_H_INCLUDED */

