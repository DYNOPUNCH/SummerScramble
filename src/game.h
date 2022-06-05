/*
 * game.c <SummerScramble>
 * 
 * boilerplate and other magic is organized here
 * 
 */

#ifndef SUMMER_GAME_H_INCLUDED
#define SUMMER_GAME_H_INCLUDED

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/* opaque types */
struct Game;
struct Texture;

struct Game *GameNew(void);
void GameInit(struct Game *g);
bool GameStep(struct Game *g);
void GameDraw(struct Game *g);
void GameCleanup(struct Game *game);
void GameDelete(struct Game *game);

/*
 *
 * general purpose functions that may be moved elsewhere
 *
 */

enum MouseState
{
	MouseState_None = 0
	, MouseState_Hover
	, MouseState_Down
	, MouseState_Clicked
};

void eztext(float x, float y, unsigned color, const char *fmt, ...);
void eztextrect(float x, float y, unsigned color, unsigned rectcolor, const char *fmt, ...);
void ezrect(float x, float y, float w, float h, unsigned color);
bool MouseInRect(float rx, float ry, float rw, float rh);
enum MouseState MouseStateRect(float rx, float ry, float rw, float rh);
void DebugLabel(float x, float y, float w, float h, unsigned color, const char *string);
enum MouseState DebugButton(float x, float y, float w, float h, unsigned color, const char *fmt, ...);
void QueueRoom(const char *roomName);

#endif /* SUMMER_GAME_H_INCLUDED */

