/*
 * sySprite.c <SyrupEngine>
 *
 * animated sprites made easy
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sySprite.h>

/* globally scoped in this file */
static struct sySpriteContext *g = 0;

/*
 *
 * private types matching EzSpriteSheet's c99 output
 * (XXX it is important that they match!)
 *
 */

struct EzSpriteSheet
{
	const char *source;
	int width;
	int height;
};

struct EzSpriteFrame
{
	int sheet;
	int x;
	int y;
	int w;
	int h;
	int ox;
	int oy;
	unsigned ms;
	int rot;
};

struct EzSpriteAnimation
{
	struct EzSpriteFrame *frame;
	const char *name;
	int frames;
	unsigned ms;
};

struct EzSpriteBank
{
	struct EzSpriteSheet *sheet;
	struct EzSpriteAnimation *animation;
	int animations;
	int sheets;
};

/*
 *
 * end EzSpriteSheet types
 *
 */

void sySpriteContextCleanup(struct sySpriteContext *ctx)
{
	void **sheetData;
	const struct EzSpriteBank *bank;
	int i;
	
	assert(ctx);
	
	sheetData = ctx->EzSpriteSheetData.sheetData;
	bank = ctx->EzSpriteSheetData.EzSpriteBank;
	
	if (!sheetData)
		return;
	
	for (i = 0; i < bank->sheets; ++i)
		ctx->texture.free(ctx->udata, sheetData[i]);
	
	free(sheetData);
	
	ctx->EzSpriteSheetData.sheetData = 0;
}
 
void sySpriteContextLoad(struct sySpriteContext *ctx)
{
	assert(ctx);
	
	if (!ctx->EzSpriteSheetData.sheetData)
	{
		void **sheetData;
		const struct EzSpriteBank *bank = ctx->EzSpriteSheetData.EzSpriteBank;
		int sheets;
		int i;
		
		assert(bank);
		assert(bank->sheet);
		assert(bank->sheets);
		
		sheets = bank->sheets;
		
		sheetData = calloc(sheets, sizeof(void*));
		
		assert(sheetData);
		
		for (i = 0; i < sheets; ++i)
		{
			struct EzSpriteSheet *sheet = &bank->sheet[i];
			
			sheetData[i] = ctx->texture.load(
				ctx->udata
				, sheet->source
				, sheet->width
				, sheet->height
			);
			
			assert(sheetData[i]);
		}
		
		ctx->EzSpriteSheetData.sheetData = sheetData;
	}
}

void sySpriteContextUse(struct sySpriteContext *ctx)
{
	assert(ctx);
	
	g = ctx;
	
	if (!g->EzSpriteSheetData.sheetData)
		sySpriteContextLoad(g);
}

void sySpriteInit(struct sySprite *sprite, void *owner, void *udata)
{
	assert(sprite);
	assert(sprite->has_init == false && "sySpriteInit multiple times?");
	
	/* populate defaults */
	memset(sprite, 0, sizeof(*sprite));
	sprite->owner = owner;
	sprite->udata = udata;
	sprite->has_init = true;
}

struct sySprite *sySpriteNewPointer(void *owner, void *udata)
{
	struct sySprite *sprite = calloc(1, sizeof(*sprite));
	
	assert(sprite);
	
	sySpriteInit(sprite, owner, udata);
	
	return sprite;
}

struct sySprite sySpriteNewValue(void *owner, void *udata)
{
	struct sySprite sprite = {0};
	
	sySpriteInit(&sprite, owner, udata);
	
	return sprite;
}

void sySpriteClearCallbacks(struct sySprite *sprite)
{
	assert(sprite);
	assert(sprite->has_init);
	
	sprite->onLoop = 0;
	sprite->onFinish = 0;
	sprite->onInterval = 0;
	sprite->onFrameAdvance = 0;
}

void sySpriteSetAnimation(struct sySprite *sprite, const char *name)
{
	const struct EzSpriteBank *bank;
	const struct EzSpriteAnimation *animation;
	
	assert(sprite);
	assert(sprite->has_init);
	
	assert(name);
	assert(g);
	
	bank = g->EzSpriteSheetData.EzSpriteBank;
	assert(bank);
	animation = bank->animation;
	assert(animation);
	
	/* restore defaults + preserve the contents of certain variables */
	*sprite = (struct sySprite)
	{
		.owner = sprite->owner
		, .udata = sprite->udata
		, .speed = 1
		, .has_init = true
	};
	
	for (animation = bank->animation
		; animation < bank->animation + bank->animations
		; ++animation
	)
		if (!strcmp(animation->name, name))
			break;
	
	assert(animation < bank->animation + bank->animations
		&& "animation not found"
	);
	
	sprite->anim = animation;
}

void sySpriteSetAnimationSeamless(struct sySprite *sprite, const char *name)
{
	struct sySprite spriteOld;
	
	assert(sprite);
	assert(name);
	
	/* save state */
	spriteOld = *sprite;
	
	/* load new animation */
	sySpriteSetAnimation(sprite, name);
	
	/* restore state */
	spriteOld.anim = sprite->anim;
	*sprite = spriteOld;
}

void sySpriteStep(struct sySprite *sprite)
{
	const struct EzSpriteAnimation *anim;
	const struct EzSpriteFrame *frame;
	unsigned long elapsed = 0;
	unsigned long now;
	unsigned short frame_index_old;
	bool has_looped = false;
	
	assert(sprite);
	assert(sprite->has_init);
	
	frame_index_old = sprite->frame_index;
	
	assert(g);
	
	anim = sprite->anim;
	assert(anim);
	assert(anim->frame);
	
	sprite->ms_scaled += g->delta_milliseconds * sprite->speed;
	
	now = sprite->ms_scaled;
	if (now >= anim->ms)
	{
		int loop_times = now / anim->ms;
		
		if (sprite->max_times == 0 || loop_times < sprite->max_times)
			now %= anim->ms;
		else
			now = anim->ms - 1;
		
		has_looped = (loop_times != sprite->looped_times);
		
		sprite->looped_times = loop_times;
	}
	
	for (frame = anim->frame; frame < anim->frame + anim->frames; ++frame)
	{
		if (now >= elapsed && now <= elapsed + frame->ms)
			break;
		
		elapsed += frame->ms;
	}
	
	sprite->frame_index = frame - anim->frame;
	
	if (sprite->frame_index != frame_index_old && sprite->onFrameAdvance)
		sprite->onFrameAdvance(sprite);
	
	if (has_looped)
	{
		if (sprite->onLoop)
			sprite->onLoop(sprite);
			
		if (sprite->onFinish && sprite->looped_times >= sprite->max_times)
			sprite->onFinish(sprite);
	}
}

void sySpriteDrawExt(const struct sySprite *sprite, float x, float y, float xscale, float yscale, float degrees, unsigned rgba)
{
	void **sheetData;
	const struct EzSpriteAnimation *anim;
	const struct EzSpriteFrame *frame;
	float xOld = x;
	float yOld = y;
	int frame_index;
	int wOff;
	int hOff;
	bool crop_rotate_90;
	bool mirror_x = false;
	bool mirror_y = false;
	
	assert(sprite);
	assert(sprite->has_init);
	assert(g);
	
	sheetData = g->EzSpriteSheetData.sheetData;
	assert(sheetData);
	
	anim = sprite->anim;
	assert(anim);
	assert(anim->frame);
	
	frame_index = sprite->frame_index % anim->frames;
	
	frame = &anim->frame[frame_index];
	
	crop_rotate_90 = !!frame->rot;
	if (xscale < 0)
	{
		mirror_x = true;
		xscale *= -1;
	}
	if (yscale < 0)
	{
		mirror_y = true;
		yscale *= -1;
	}
	
	/* swap mirror offset axes for rotated images */
	if (crop_rotate_90)
	{
		wOff = frame->h;
		hOff = frame->w;
	}
	else
	{
		wOff = frame->w;
		hOff = frame->h;
	}
	
	/* x mirroring */
	if (mirror_x)
		x += frame->ox - wOff;
	else
		x -= frame->ox;
	
	/* y mirroring */
	if (mirror_y)
		y += frame->oy - hOff;
	else
		y -= frame->oy;
	
	// TODO xscale and yscale	
	
	g->texture.draw(
		g->udata
		, sheetData[frame->sheet]
		, x
		, y
		, xOld
		, yOld
		, degrees
		, mirror_x
		, mirror_y
		, frame->x
		, frame->y
		, frame->w
		, frame->h
		, crop_rotate_90
	);
}

void (sySpriteDraw)(struct sySpriteDrawParams params)
{
	sySpriteDrawExt(
		params.sprite
		, params.x
		, params.y
		, params.xscale
		, params.yscale
		, params.degrees
		, params.rgba
	);
}
