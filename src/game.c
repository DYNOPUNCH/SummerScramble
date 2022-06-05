/*
 * game.c <SummerScramble>
 * 
 * boilerplate and other magic is organized here
 * 
 */

#include <assert.h>
#include <SDL2/SDL.h>
#include <EzSprite.h>
#include <stb_image.h>
#include <math.h>

//#define USE_VFS

#include <syOgmo.h>
#include <syFile.h>

#include "game.h"
#include "common.h"

#define GAME_NAME "Summer Scramble"
#define GAME_VER ": prototyping stage"
#define SDL_ERR(X) die(X " error: %s", SDL_GetError())

#define WIN_W (480)
#define WIN_H (270)

#define SDL_EZTEXT_IMPLEMENTATION
#include <SDL_EzText.h>

/* room database */
#include "tmp/entity-ids.h"
#include "tmp/entity-all.h"
static struct syOgmoRoom roomDatabase[] =
{
	#include "tmp/rooms.h"
};
const struct syOgmoEntityClass *entityClassDatabase[] =
{
	#include "tmp/entity-classes.h"
};

/*
 *
 * private types
 *
 */

struct Texture
{
	struct
	{
		SDL_Texture  *main;
		int width;
		int height;
	} SDL;
};

struct Mouse
{
	int x;
	int y;
	bool press;
	bool release;
	unsigned long releaseTicks;
};

struct Game
{
	struct
	{
		SDL_Window    *window;
		SDL_Renderer  *renderer;
		struct
		{
			SDL_Texture *fullscreen;
		} target;
	} SDL;
	struct
	{
		struct
		{
			int w;
			int h;
		} fullscreen;
	} window;
	struct EzSpriteContext *spriteCtx;
	int scale;
	bool isFullscreen;
	bool isFullscreenDisabled;
	struct syRoom *room;
	struct Mouse mouse;
	const char *queuedRoomName;
	struct
	{
		int x;
		int y;
		int w;
		int h;
	} fullscreenRect;
};

/* XXX globally scoped pointer to game */
struct Game *gGame = 0;


/*
 *
 * private functions
 *
 */


/* loads pixel data from image file */
static void *pixels_load(const char *fn, int *width, int *height)
{
	const void *data;
	size_t dataSz;
	bool isCopy;
	void *pix;
	int width_;
	int height_;
	int n;
	
	if (!width)
		width = &width_;
	if (!height)
		height = &height_;
	
	assert(fn);
	
	data = syFileLoadReadonly(fn, &dataSz, &isCopy);
	
	/* load image pixels using stb */
	if (!(pix = stbi_load_from_memory(data, dataSz, width, height, &n, STBI_rgb_alpha)))
	{
		die("failed to load pixels from file '%s'", fn);
		return 0;
	}
	
	if (isCopy)
		syFileFree(data);
	
	return pix;
}

static void pixels_free(void *pix)
{
	stbi_image_free(pix);
}

static void *texture_make(void *udata, void *pix, int width, int height)
{
	struct Game *game = udata;
	struct Texture *t = calloc(1, sizeof(*t));
	SDL_Texture *tex;
	SDL_Surface *surf;
	uint32_t rmask;
	uint32_t gmask;
	uint32_t bmask;
	uint32_t amask;
	int depth = 32;
	
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
	#else /* LE */
	amask = 0xff000000;
	bmask = 0x00ff0000;
	gmask = 0x0000ff00;
	rmask = 0x000000ff;
	#endif
	
	assert(game);
	assert(t);
	
	t->SDL.width = width;
	t->SDL.height = height;
	
	if (pix) /* image texture */
	{
		/* convert pixels to surface */
		if (!(surf = SDL_CreateRGBSurfaceFrom(
			pix, width, height, depth, 4 * width
			, rmask, gmask, bmask, amask
		)))
			return 0;
		
		/* convert surface to texture */
		if (!(tex = SDL_CreateTextureFromSurface(game->SDL.renderer, surf)))
		{
			SDL_FreeSurface(surf);
			return 0;
		}
		
		SDL_FreeSurface(surf);
	}
	else /* framebuffer texture */
	{
		tex = SDL_CreateTexture(
			game->SDL.renderer
			, SDL_PIXELFORMAT_RGBA8888
			, SDL_TEXTUREACCESS_TARGET
			, width
			, height
		);
	}
	t->SDL.main = tex;
	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	
	return t;
}

/*
 * 
 * a light binding enabling EzSprite to communicate
 * 
 */
static void *texture_load(
	void *udata
	, const char *fn
	, int width
	, int height
)
{
	struct Texture *t;
	void *pix;
	int x;
	int y;
	
	/* load image pixels using stb */
	if (!(pix = pixels_load(fn, &x, &y)))
		return 0;
	
	if (width <= 0)
		width = x;
	if (height <= 0)
		height = y;
	
	assert(x == width);
	assert(y == height);
	
	t = texture_make(udata, pix, x, y);
	assert(t);
	
	/* cleanup */
	pixels_free(pix);
	
	return t;
}

static void texture_free(
	void *udata
	, void *texture
)
{
	struct Game *game = udata;
	struct Texture *t = texture;
	
	(void)game;
	assert(t);
	
	if (t->SDL.main)
		SDL_DestroyTexture(t->SDL.main);
}

static void texture_clear(void *udata, void *fb, int x, int y, int w, int h)
{
	struct Game *game = udata;
	struct Texture *t = fb;
	
	(void)game;
	assert(t);
	
	if (!t->SDL.main)
		return;
	
	if (!w || !h)
	{
		w = t->SDL.width;
		h = t->SDL.height;
	}
	
	{
		SDL_Renderer *ren = game->SDL.renderer;
		SDL_Texture *ofb;
		SDL_BlendMode blendMode;
		Uint8 r, g, b, a;
		SDL_Rect rect = {x, y, w, h};
		
		/* save state */
		ofb = SDL_GetRenderTarget(ren);
		SDL_GetRenderDrawBlendMode(ren, &blendMode);
		SDL_GetRenderDrawColor(ren, &r, &g, &b, &a);
		
		/* clear region */
		SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
		SDL_SetRenderTarget(ren, t->SDL.main);
		SDL_RenderFillRect(ren, &rect);
		
		/* restore state */
		SDL_SetRenderTarget(ren, ofb);
		SDL_SetRenderDrawBlendMode(ren, blendMode);
		SDL_SetRenderDrawColor(ren, r, g, b, a);
	}
}

static void texture_draw(
	void *udata
	, void *texture
	, float x
	, float y
	, int xflip
	, int yflip
	, int cx
	, int cy
	, int cw
	, int ch
	, int rotate
)
{
	struct Texture *t = texture;
	struct Game *game = udata;
	SDL_Texture *which;
	SDL_Rect src = {cx, cy, cw, ch};
	SDL_Rect dst = {x, y, cw, ch};
	SDL_RendererFlip flip = 0;
	
	dst.x = roundf(game->scale * x);
	dst.y = roundf(game->scale * y);
	dst.w = roundf(game->scale * dst.w);
	dst.h = roundf(game->scale * dst.h);
	
	if (xflip)
		flip |= SDL_FLIP_HORIZONTAL;
	if (yflip)
		flip |= SDL_FLIP_VERTICAL;
	
	assert(t);
	which = t->SDL.main;
	
	/* SDL2's builtin sprite rotation isn't crisp, so
	 * complain if there are rotated sprites
	 */
	assert(rotate == 0);
	
	SDL_RenderCopyEx(game->SDL.renderer
		, which
		, &src
		, &dst
		, 0
		, 0
		, flip
	);
}

static void texture_draw1x(
	void *udata
	, void *texture
	, float x
	, float y
	, int xflip
	, int yflip
	, int cx
	, int cy
	, int cw
	, int ch
	, int rotate
)
{
	struct Texture *t = texture;
	struct Game *game = udata;
	SDL_Texture *which;
	SDL_Rect src = {cx, cy, cw, ch};
	SDL_Rect dst = {x, y, cw, ch};
	SDL_RendererFlip flip = 0;
	
	if (xflip)
		flip |= SDL_FLIP_HORIZONTAL;
	if (yflip)
		flip |= SDL_FLIP_VERTICAL;
	
	assert(t);
	which = t->SDL.main;
	
	/* SDL2's builtin sprite rotation isn't crisp, so
	 * complain if there are rotated sprites
	 */
	assert(rotate == 0);
	
	SDL_RenderCopyEx(game->SDL.renderer
		, which
		, &src
		, &dst
		, 0
		, 0
		, flip
	);
}

static void texture_drawonto( /* crop from texture onto framebuffer */
	void *udata
	, void *texture
	, void *framebuffer
	, int x
	, int y
	, int cx
	, int cy
	, int cw
	, int ch
)
{
	struct Texture *t = texture;
	struct Texture *fb = framebuffer;
	struct Game *game = udata;
	SDL_Texture *which;
	SDL_Texture *ofb;
	SDL_Rect src = {cx, cy, cw, ch};
	SDL_Rect dst = {x, y, cw, ch};
	SDL_Renderer *ren = game->SDL.renderer;
	
	assert(t);
	assert(framebuffer);
	which = t->SDL.main;
	
	ofb = SDL_GetRenderTarget(ren);
	SDL_SetRenderTarget(ren, fb->SDL.main);
	SDL_RenderCopy(ren, which, &src, &dst);
	SDL_SetRenderTarget(ren, ofb);
}

static unsigned long ticks(void *udata)
{
	(void)udata;
	
	return SDL_GetTicks();
}

static void *ogmo_decal_image_load(struct Game *game, const char *fn)
{
	struct Texture *tex = texture_load(game, fn, 0, 0);
	
	assert(tex);
	
	return tex;
}
static void ogmo_decal_image_draw(struct Game *game, void *img, float ulx, float uly)
{
	struct Texture *tex = img;
	
	assert(tex);
	
	texture_draw(game
		, tex
		, ulx, uly
		, 0, 0 /* flip */
		, 0, 0, tex->SDL.width, tex->SDL.height /* crop xywh */
		, 0
	);
}
static void ogmo_decal_image_free(struct Game *game, void *img)
{
	struct Texture *tex = img;
	
	assert(tex);
	
	if (tex->SDL.main)
		SDL_DestroyTexture(tex->SDL.main);
	
	free(tex);
}
static int ogmo_decal_image_get_width(struct Game *game, void *img)
{
	struct Texture *tex = img;
	
	assert(tex);
	
	return tex->SDL.width;
}
static int ogmo_decal_image_get_height(struct Game *game, void *img)
{
	struct Texture *tex = img;
	
	assert(tex);
	
	return tex->SDL.height;
}

/*
 *
 * public functions
 *
 */

/* returns the maximum scaling factor a window should use */
unsigned GameGetWindowMaxSize(struct Game *game)
{
	SDL_DisplayMode dm;
	int w = 0;
	int h = 0;
	int i;
	
	assert(game);
	assert(game->SDL.window);
	
	/* get the largest display mode of any monitor for now */
	for (i = 0; i < SDL_GetNumVideoDisplays(); ++i)
	{
		if (SDL_GetCurrentDisplayMode(i, &dm))
			SDL_ERR("SDL_GetCurrentDisplayMode");
		
		if (dm.w * dm.h > w * h)
		{
			w = dm.w;
			h = dm.h;
		}
	}
	
	if (w > h)
		return w / WIN_W - 1;
	else
		return h / WIN_H - 1;
	
	(void)game;
}

/* get fullscreen scaling factor */
unsigned GameGetFullscreenSize(struct Game *game)
{
	SDL_DisplayMode dm;
	unsigned scale = 0;
	
	assert(game);
	assert(game->SDL.window);
	
	if (SDL_GetWindowDisplayMode(game->SDL.window, &dm))
		SDL_ERR("SDL_GetWindowDisplayMode");
	
	game->window.fullscreen.w = dm.w;
	game->window.fullscreen.h = dm.h;
	
	if (dm.w > dm.h)
		do ++scale; while (WIN_W * scale < dm.w);
	else
		do ++scale; while (WIN_H * scale < dm.h);
	
	return scale;
}

/* increment (1) or decrement (-1) window size (0 = simple refresh) */
void GameUpdateWindowSize(struct Game *game, int n)
{
	unsigned scaleMax;
	unsigned scale;
	
	assert(game);
	assert(n == 1 || n == -1 || n == 0);
	
	if (game->isFullscreen)
		return;
	
	scaleMax = GameGetWindowMaxSize(game);
	
	game->scale += n;
	
	/* keep values in this range */
	if (game->scale < 1)
		game->scale = 1;
	else if (game->scale > scaleMax)
		game->scale = scaleMax;
	
	scale = game->scale;
	
	SDL_SetWindowSize(game->SDL.window, WIN_W * scale, WIN_H * scale);
}

struct Game *GameNew(void)
{
	struct Game *game;
	
	return calloc(1, sizeof(*game));
}

void GameDraw(struct Game *g)
{
	SDL_Renderer *renderer = g->SDL.renderer;
	
	/* prepare drawing destination */
	if (g->isFullscreen)
		SDL_SetRenderTarget(renderer, g->SDL.target.fullscreen);
	else
		SDL_SetRenderTarget(renderer, 0);
	SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, -1);
	SDL_RenderClear(renderer);
	
	/* draw game world */
	if (g->room)
		syRoomDraw(g->room);
	
	/* fullscreen logic happens after drawing */
	if (g->isFullscreen)
	{
		SDL_Rect dst = {0, 0, g->window.fullscreen.w, g->window.fullscreen.h};
		
		/* letterboxing and pillarboxing logic */
		{
			float hOverW = (float)WIN_H / WIN_W;
			float wOverH = (float)WIN_W / WIN_H;
			SDL_Rect letterbox = {
				.w = g->window.fullscreen.w
				, .h = hOverW * g->window.fullscreen.w
			};
			SDL_Rect pillarbox = {
				.w = wOverH * g->window.fullscreen.h
				, .h = g->window.fullscreen.h
			};
			
			/* select and center the one with the smaller picture */
			if (letterbox.w * letterbox.h < pillarbox.w * pillarbox.h)
			{
				dst = letterbox;
				dst.y = (g->window.fullscreen.h - dst.h) / 2;
			}
			else
			{
				dst = pillarbox;
				dst.x = (g->window.fullscreen.w - dst.w) / 2;
			}
		}
		
		/* copy letterboxed/pillarboxed result to destination */
		SDL_SetRenderTarget(renderer, 0);
		SDL_RenderSetClipRect(renderer, 0);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, -1);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, g->SDL.target.fullscreen, 0, &dst);
		
		/* store letterboxing/pillarboxing info for later */
		g->fullscreenRect.x = dst.x;
		g->fullscreenRect.y = dst.y;
		g->fullscreenRect.w = dst.w;
		g->fullscreenRect.h = dst.h;
	}
	
	/* and then display it onto the screen */
	SDL_RenderPresent(renderer);
}

bool GameStep(struct Game *g)
{
	/*
	 * handle events
	 */
	SDL_Event event;
	
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				return true;
			
			case SDL_MOUSEMOTION:
				g->mouse.x = event.motion.x;
				g->mouse.y = event.motion.y;
				
				if (g->isFullscreen)
				{
					if (!g->fullscreenRect.w)
						g->fullscreenRect.w = 1;
					if (!g->fullscreenRect.h)
						g->fullscreenRect.h = 1;
					g->mouse.x -= g->fullscreenRect.x;
					g->mouse.y -= g->fullscreenRect.y;
					g->mouse.x /= (float)g->fullscreenRect.w / WIN_W;
					g->mouse.y /= (float)g->fullscreenRect.h / WIN_H;
				}
				else
				{
					g->mouse.x /= g->scale;
					g->mouse.y /= g->scale;
				}
				break;
			
			case SDL_MOUSEBUTTONDOWN:
				g->mouse.press = true;
				break;
			
			case SDL_MOUSEBUTTONUP:
				g->mouse.press = false;
				g->mouse.release = true;
				g->mouse.releaseTicks = ticks(g);
				break;
			
			/* key press */
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					/* quick exit */
					case SDLK_ESCAPE:
						return true;
					
					/* shrink window */
					case SDLK_F1:
						GameUpdateWindowSize(g, -1);
						break;
					
					/* inflate window */
					case SDLK_F2:
						GameUpdateWindowSize(g, 1);
						break;
					
					/* fullscreen window */
					case SDLK_F11:
					{
						static int windowedScale;
						static int windowedX;
						static int windowedY;
						int toggle = SDL_WINDOW_FULLSCREEN_DESKTOP;
						if (g->isFullscreenDisabled)
							break;
						if (g->isFullscreen)
							toggle = 0;
						g->isFullscreen = !g->isFullscreen;
						
						/* back up non-fullscreen window stats before switching */
						if (g->isFullscreen)
						{
							windowedScale = g->scale;
							SDL_GetWindowPosition(g->SDL.window, &windowedX, &windowedY);
						}
						
						/* toggle window type */
						if (SDL_SetWindowFullscreen(g->SDL.window, toggle))
						{
							g->isFullscreenDisabled = true;
							break;
						}
						
						/* set up fullscreen */
						if (g->isFullscreen)
						{
							static int fullscreen_size = 0;
							g->scale = GameGetFullscreenSize(g) + 1;
							if (!g->SDL.target.fullscreen
								|| g->scale != fullscreen_size /* resize */
							)
							{
								if (g->SDL.target.fullscreen)
									SDL_DestroyTexture(g->SDL.target.fullscreen);
								
								/* want anti-aliasing on downscale */
								SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
								g->SDL.target.fullscreen = SDL_CreateTexture(
									g->SDL.renderer
									, SDL_PIXELFORMAT_RGBA8888
									, SDL_TEXTUREACCESS_TARGET
									, WIN_W * g->scale
									, WIN_H * g->scale
								);
								SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
							}
							fullscreen_size = g->scale;
						}
						
						/* restore to windowed mode */
						else
						{
							g->scale = windowedScale;
							GameUpdateWindowSize(g, 0);
							SDL_SetWindowPosition(g->SDL.window, windowedX, windowedY);
						}
						break;
					}
				}
				break;
		}
	}
	
	/*
	 * actually step forward
	 */
	
	if (g->queuedRoomName)
	{
		if (g->room)
			syRoomDelete(g->room);
		g->room = syRoomNewFromFilename(g->queuedRoomName);
		g->queuedRoomName = 0;
	}
	
	return false;
}

#ifdef USE_VFS
	struct syFileVirtual vfs[] =
	{
		#include "tmp/vfs.h"
	};
#endif

#define EZSPRITESHEET_NAME gfx
#include "tmp/gfx.h"

void GameInit(struct Game *g)
{
	assert(g);
	gGame = g; /* XXX */
	
	#ifdef USE_VFS
	syFileUseVFS(vfs, ARRAY_COUNT(vfs));
	#endif
	
	/* initialization */
	g->scale = 1;
	SDL_Init(SDL_INIT_EVERYTHING);
	g->SDL.window = SDL_CreateWindow(
		GAME_NAME GAME_VER
		, SDL_WINDOWPOS_CENTERED
		, SDL_WINDOWPOS_CENTERED
		, WIN_W
		, WIN_H
		, SDL_WINDOW_SHOWN
	);
	g->SDL.renderer = SDL_CreateRenderer(
		g->SDL.window
		, -1
		, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);
	SDL_SetRenderDrawBlendMode(g->SDL.renderer, SDL_BLENDMODE_BLEND);
	g->spriteCtx = EzSpriteContext_new((struct EzSpriteContextInit){
		.udata = g
		, .texture = {
			.load = texture_load
			, .free = texture_free
			, .draw = texture_draw
		}
		, .ticks = ticks
	});
	EzSpriteContext_addbank(g->spriteCtx, &gfx);
	EzSpriteContext_loaddeps(g->spriteCtx);
	
	/* room system */
	{
		syRoomContextSetGame(g);
		syRoomContextSetError(die);
		syRoomContextSetOgmoDatabase(roomDatabase, ARRAY_COUNT(roomDatabase));
		syRoomContextSetEntityClassDatabase(entityClassDatabase, ARRAY_COUNT(entityClassDatabase));
		
		syRoomContextSetImageLoad(ogmo_decal_image_load);
		syRoomContextSetImageDraw(ogmo_decal_image_draw);
		syRoomContextSetImageFree(ogmo_decal_image_free);
		syRoomContextSetImageGetWidth(ogmo_decal_image_get_width);
		syRoomContextSetImageGetHeight(ogmo_decal_image_get_height);
		
		g->room = syRoomNewFromFilename("test");
		//g->room = syRoomNewFromFilename("apartment/pool");
		//g->room = syRoomNewFromFilename("apartment/crime-scene");
	}
}

void GameCleanup(struct Game *g)
{
	/* cleanup */
	EzSpriteContext_delete(g->spriteCtx);
	SDL_DestroyRenderer(g->SDL.renderer);
	SDL_DestroyWindow(g->SDL.window);
}

void GameDelete(struct Game *game)
{
	free(game);
}

/*
 *
 * general purpose functions that may be moved elsewhere
 *
 */

void eztext(float x, float y, unsigned color, const char *fmt, ...)
{
	char buf[4096];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	SDL__EzText.color = color;
	SDL__EzText.multiplier = gGame->scale;
	SDL_EzText(gGame->SDL.renderer, x, y, buf);
	va_end(args);
}

void eztextrect(float x, float y, unsigned color, unsigned rectcolor, const char *fmt, ...)
{
	char buf[4096];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	SDL__EzText.color = color;
	SDL__EzText.rect = rectcolor;
	SDL__EzText.multiplier = gGame->scale;
	SDL_EzTextRect(gGame->SDL.renderer, x, y, buf);
	va_end(args);
}

void ezrect(float x, float y, float w, float h, unsigned color)
{
	SDL_Renderer *ren = gGame->SDL.renderer;
	SDL_Rect r = {
		roundf(gGame->scale * x)
		, roundf(gGame->scale * y)
		, roundf(gGame->scale * w)
		, roundf(gGame->scale * h)
	};
	
	SDL_SetRenderDrawColor(ren, color >> 24, color >> 16, color >> 8, color);
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(ren, &r);
}

bool MouseInRect(float rx, float ry, float rw, float rh)
{
	float scale = 1;//gGame->scale;
	return (gGame->mouse.x >= (scale * rx)
		&& gGame->mouse.y >= (scale * ry)
		&& gGame->mouse.x <= (scale * (rx + rw))
		&& gGame->mouse.y <= (scale * (ry + rh))
	);
}

enum MouseState MouseStateRect(float rx, float ry, float rw, float rh)
{
	if (!MouseInRect(rx, ry, rw, rh))
		return MouseState_None;
	
	/* timeout in milliseconds */
	if (ticks(gGame) - gGame->mouse.releaseTicks >= 30)
		gGame->mouse.release = false;
	
	if (gGame->mouse.release)
	{
		gGame->mouse.release = false;
		return MouseState_Clicked;
	}
	
	if (gGame->mouse.press)
		return MouseState_Down;
	
	return MouseState_Hover;
}

/* get the best contrasting font color against a background color */
unsigned bestFontContrast(unsigned bgcolor)
{
    unsigned char red = bgcolor >> 16;
    unsigned char green = bgcolor >> 8;
    unsigned char blue = bgcolor;
    double y = (800 * red + 587 * green + 114 * blue) / 1000;
    float brightness = MAX3(red, green, blue) * (1.0 / 255.0);

    if (brightness <= 0.7)
        return COLOR_WHITE;

    return y > 127 ? COLOR_BLACK : COLOR_WHITE;
}

unsigned ColorBlend(unsigned dst, unsigned src)
{
	unsigned char dR = dst >> 24;
	unsigned char dG = dst >> 16;
	unsigned char dB = dst >>  8;
	unsigned char dA = dst >>  0;
	unsigned char sR = src >> 24;
	unsigned char sG = src >> 16;
	unsigned char sB = src >>  8;
	unsigned char sA = src >>  0;
	
	float sAf = sA / 255.0f;
	float dAf = dA / 255.0f;
	
	dR = sR * sAf + dR * (1.0f - sAf);
	dG = sG * sAf + dG * (1.0f - sAf);
	dB = sB * sAf + dB * (1.0f - sAf);
	dA = (sAf + dAf * (1.0f - sAf)) * 255.0f;
	
	return (dR << 24) | (dG << 16) | (dB << 8) | dA;
}

void DebugLabel(float x, float y, float w, float h, unsigned color, const char *string)
{
	float strW = SDL_EzTextStringWidth(string);
	float strH = SDL_EzTextStringHeight(string);
	
	ezrect(x, y, w, h, color);
	SDL__EzText.color = bestFontContrast(color);
	SDL__EzText.multiplier = gGame->scale;
	SDL_EzText(
		gGame->SDL.renderer
		, (x + (w - strW) / 2)
		, (y + (h - strH) / 2)
		, string
	);
}

enum MouseState DebugButton(float x, float y, float w, float h, unsigned color, const char *fmt, ...)
{
	char buf[4096];
	va_list args;
	enum MouseState mstate = MouseStateRect(x, y, w, h);
	
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	if (mstate == MouseState_Hover)
		color = ColorBlend(color, 0xffffff80);
	if (mstate == MouseState_Down)
		color = ColorBlend(color, 0x00000040);
	DebugLabel(x, y, w, h, color, buf);
	va_end(args);
	
	return mstate;
}

void QueueRoom(const char *roomName)
{
	gGame->queuedRoomName = roomName;
}

