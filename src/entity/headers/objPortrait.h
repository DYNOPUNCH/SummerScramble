/* 
 * objPortrait.h <SummerScramble>
 * 
 * this object displays NPC portraits
 * 
 */

#ifndef objPortrait_H_INCLUDED
#define objPortrait_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
enum objPortraitState
{
	objPortraitState_Main
	, objPortraitState_COUNT
};
/* </ogmoenums> */

/* <ogmostruct> */
struct objPortrait
{
	/* <ogmoblock> */
	enum objPortraitState State;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
	const char *Name;
	float x;
	float y;
	struct sySprite spriteBody;
	struct sySprite spriteBlink;
	struct sySprite spriteTalk;
	int blinkCountDown;
	bool isBlinking;
	bool isTalking;
	int stackOrder;
};
/* <ogmoblock1> */
extern const struct objPortrait objPortraitDefaults;
extern const struct syOgmoEntityClass objPortraitClass;
#define objPortraitValues(...) (struct objPortrait){.State = 0, .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objPortrait_H_INCLUDED */

