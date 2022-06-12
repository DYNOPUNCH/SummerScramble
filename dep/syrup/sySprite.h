/*
 * sySprite.h <SyrupEngine>
 *
 * animated sprites made easy
 *
 */

#ifndef SYRUP_SPRITE_H_INCLUDED
#define SYRUP_SPRITE_H_INCLUDED

#include <stdbool.h>

/* types */
struct sySprite;
struct sySpriteContext;
struct sySpriteDrawParams;
typedef void (sySpriteCallback)(struct sySprite *caller);

/* functions */
void sySpriteContextLoad(struct sySpriteContext *ctx);
void sySpriteContextUse(struct sySpriteContext *ctx);
void sySpriteContextCleanup(struct sySpriteContext *ctx);
void sySpriteInit(struct sySprite *sprite, void *owner, void *udata);
struct sySprite *sySpriteNewPointer(void *owner, void *udata);
struct sySprite sySpriteNewValue(void *owner, void *udata);
void sySpriteClearCallbacks(struct sySprite *sprite);
void sySpriteSetAnimation(struct sySprite *sprite, const char *name);
void sySpriteSetAnimationSeamless(struct sySprite *sprite, const char *name);
void sySpriteStep(struct sySprite *sprite);
void sySpriteDrawExt(const struct sySprite *sprite, float x, float y, float xscale, float yscale, float degrees, unsigned rgba);
void (sySpriteDraw)(struct sySpriteDrawParams params);
#define sySpriteDraw(...) (sySpriteDraw)((struct sySpriteDrawParams){.xscale = 1, .yscale = 1, .rgba = -1,  __VA_ARGS__ })

/* type definitions */

/* initialization context; this is how the user provides a binding */
struct sySpriteContext
{
	void *udata; /* main gameplay context */
	
	struct /* texture functions */
	{
		void* (*load)( /* load a texture */
			void *udata
			, const char *fn
			, int width
			, int height
		);
		void (*free)( /* free a texture */
			void *udata
			, void *texture
		);
		void (*draw)( /* display a texture to the screen */
			void *udata
			, void *texture
			, float x
			, float y
			, float pivot_x
			, float pivot_y
			, float degrees
			, bool mirror_x
			, bool mirror_y
			, int crop_x
			, int crop_y
			, int crop_w
			, int crop_h
			, bool crop_rotate_90
		);
	} texture;
	
	/* general functions */
	double delta_milliseconds; /* time since last step */
	
	struct
	{
		const void *EzSpriteBank;
		void **sheetData;
	} EzSpriteSheetData;
};

struct sySprite
{
	/*
	 * the following variables are persistent across animation changes
	 */
	const void *anim;
	void *owner;
	void *udata;
	
	/*
	 * the following variables are reset to their defaults when
	 * sySpriteSetAnimation() is used; to preserve them while
	 * switching animations, use sySpriteSetAnimationSeamless()
	 */
	sySpriteCallback *onLoop; /* misc callbacks */
	sySpriteCallback *onFinish;
	sySpriteCallback *onInterval;
	double ms_scaled; /* milliseconds animation has advanced (XXX scaled by speed, varying) */
	float speed; /* animation playback speed (1 = default) */
	unsigned short max_times; /* number of times to play (loop) the animation (0 = infinity) */
	unsigned short looped_times; /* number of times animation has looped (internal use) */
	unsigned short frame_index; /* frame index within animation (you can hijack this) */
};

struct sySpriteDrawParams
{
	const struct sySprite *sprite;
	float x;
	float y;
	float xscale;
	float yscale;
	float degrees;
	unsigned rgba;
};

#endif /* SYRUP_SPRITE_H_INCLUDED */
