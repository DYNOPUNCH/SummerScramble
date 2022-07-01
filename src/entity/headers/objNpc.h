/* 
 * objNpc.h <SummerScramble>
 * 
 * the primary NPC object
 * 
 */

#ifndef objNpc_H_INCLUDED
#define objNpc_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
enum objNpcState
{
	objNpcState_Main
	, objNpcState_COUNT
};
/* </ogmoenums> */

/* <ogmostruct> */
struct objNpc
{
	/* <ogmoblock> */
	enum objNpcState State;
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
extern const struct objNpc objNpcDefaults;
extern const struct syOgmoEntityClass objNpcClass;
#define objNpcValues(...) (struct objNpc){.State = 0, .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objNpc_H_INCLUDED */

