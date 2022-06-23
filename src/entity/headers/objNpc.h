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
enum objNpcWhich
{
	objNpcWhich_Test
	, objNpcWhich_COUNT
};
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
	enum objNpcWhich Which;
	enum objNpcState State;
	syOgmoEntityFunc OnInit;
	syOgmoEntityFunc OnClick;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
	char Name[32];
	struct sySprite spriteBody;
	struct sySprite spriteBlink;
	struct sySprite spriteTalk;
	bool isBlinking;
	bool isTalking;
};
/* <ogmoblock1> */
extern const struct objNpc objNpcDefaults;
extern const struct syOgmoEntityClass objNpcClass;
#define objNpcValues(...) (struct objNpc){.Which = 0, .State = 0, .OnInit = __objNpcUniqueFunc0, .OnClick = __objNpcUniqueFunc0, .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objNpc_H_INCLUDED */

