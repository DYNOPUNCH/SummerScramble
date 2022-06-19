/* 
 * objAnimationTest.c <ProjectName>
 * 
 * your description here
 * 
 */

#include <objAnimationTest.h>

#include <SDL2/SDL.h>

/* <ogmodefaults> */
const struct objAnimationTest objAnimationTestDefaults = 
{
	/* <ogmoblock> */
	.new_value = false
	, .unused___ = 0
	/* </ogmoblock> */
	
	/* <ogmoblock1> */
	/* defaults for custom user-defined variables go here */
	/* </ogmoblock1> */
};
/* </ogmodefaults> */

/* <ogmonew> */
static void *New(const void *src)
{
	/* <ogmoblock> */
	struct objAnimationTest *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objAnimationTestDefaults;
	
	memcpy(my, src, sizeof(*my));
	/* </ogmoblock> */
	
	/* custom code goes here */
	
	/* <ogmoblock1> */
	return my;
	/* </ogmoblock1> */
}
/* </ogmonew> */

syOgmoEntityFuncDecl(Init)
{
	struct objAnimationTest *my = ogmo->values;
	
	debug("init objAnimationTest, my = %p\n", my);
	
	/* init */
	sySpriteInit(&my->spriteBase, my, 0);
	sySpriteInit(&my->spriteEyes, my, 0);
	sySpriteInit(&my->spriteMouth, my, 0);
	sySpriteSetAnimationString(&my->spriteBase, "npc-test/base");
	sySpriteSetAnimationString(&my->spriteEyes, "npc-test/stare");
	sySpriteSetAnimationString(&my->spriteMouth, "npc-test/talk");
	
	return 0;
}

static void blinkFinished(struct sySprite *caller)
{
	struct objAnimationTest *owner;
	
	owner = caller->owner;
	
	debug("blinkFinished owner = %p\n", owner);
	debug("blinkFinished caller = %p\n", caller);
	
	sySpriteSetAnimationString(&owner->spriteEyes, "npc-test/stare");
	
	// or you can just do this
	//sySpriteSetAnimation(caller, "npc-test/stare");
}

syOgmoEntityFuncDecl(Step)
{
	struct objAnimationTest *my = ogmo->values;
	
	/* step */
	sySpriteStep(&my->spriteBase);
	sySpriteStep(&my->spriteEyes);
	sySpriteStep(&my->spriteMouth);
	
	if (SDL_GetTicks() - my->blinkStart > 2000)
	{
		my->blinkStart = SDL_GetTicks();
		sySpriteSetAnimationString(&my->spriteEyes, "npc-test/blink");
		my->spriteEyes.onFinish = blinkFinished;
		my->spriteEyes.max_times = 1;
	}
	
	return 0;
}

syOgmoEntityFuncDecl(Draw)
{
	struct objAnimationTest *my = ogmo->values;
	
	sySpriteDraw(.sprite = &my->spriteBase, .x = ogmo->x, .y = ogmo->y);
	sySpriteDraw(.sprite = &my->spriteEyes, .x = ogmo->x, .y = ogmo->y);
	sySpriteDraw(.sprite = &my->spriteMouth, .x = ogmo->x, .y = ogmo->y);
	
	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objAnimationTestClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Init] = Init
		, [syOgmoExec_Step] = Step
		, [syOgmoExec_Draw] = Draw
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Init] = 0
		, [syOgmoExec_Step] = 0
		, [syOgmoExec_Draw] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

