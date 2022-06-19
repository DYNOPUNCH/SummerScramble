/* 
 * objNpc.c <SummerScramble>
 * 
 * the primary NPC object
 * 
 */

#include <objNpc.h>

/* <ogmodefaults> */
const struct objNpc objNpcDefaults = 
{
	/* <ogmoblock> */
	.Which = 0
	, .State = 0
	, .unused___ = 0
	/* </ogmoblock> */
	
	/* <ogmoblock1> */
	/* defaults for custom user-defined variables go here */
	/* </ogmoblock1> */
};
/* </ogmodefaults> */

/* private functions */
static char *GetName(struct objNpc *my)
{
	const char *tmp = "";
	
	if (my->Name[0])
		return my->Name;
	
	switch (my->Which)
	{
		case objNpcWhich_Test: tmp = "Test"; break;
		default: tmp = "Unknown"; break;
	}
	
	strncpy(my->Name, tmp, sizeof(my->Name));
	
	return my->Name;
}

static const char *Fmt(const char *fmt, ...)
{
	static char tmp[1024];
	
	va_list args;
	va_start(args, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, args);
	va_end(args);
	
	return tmp;
}

static void BlinkOnLoop(struct sySprite *caller)
{
	struct objNpc *owner;
	
	owner = caller->owner;
	
	assert(owner);
	
	/* TODO randomized blinking; for now, it alternates on/off each loop */
	owner->isBlinking = !owner->isBlinking;
}

static void SetupFrames(struct objNpc *my, const char *StateStr)
{
	GetName(my);
	
	if (!StateStr)
		switch (my->State)
		{
			case objNpcState_Main: StateStr = "Main"; break;
			default: StateStr = ""; break;
		}
	
#define S(FEATURE) sySpriteSetAnimationString(&my->sprite##FEATURE, Fmt("Npc/%s/%s/%s", my->Name, #FEATURE, StateStr))
	S(Body);
	S(Blink);
	S(Talk);
	my->spriteBlink.onLoop = BlinkOnLoop;
#undef S
}

/* <ogmonew> */
static void *New(const void *src)
{
	/* <ogmoblock> */
	struct objNpc *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objNpcDefaults;
	
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
	struct objNpc *my = ogmo->values;
	
	/* init */
	sySpriteInit(&my->spriteBody, my, 0);
	sySpriteInit(&my->spriteBlink, my, 0);
	sySpriteInit(&my->spriteTalk, my, 0);
	
	SetupFrames(my, 0);
	
	return 0;
}

syOgmoEntityFuncDecl(Step)
{
	struct objNpc *my = ogmo->values;
	
	/* step */
	sySpriteStep(&my->spriteBody);
	sySpriteStep(&my->spriteBlink);
	sySpriteStep(&my->spriteTalk);
	
	return 0;
}

syOgmoEntityFuncDecl(Draw)
{
	struct objNpc *my = ogmo->values;
	
	sySpriteDraw(.sprite = &my->spriteBody, .x = ogmo->x, .y = ogmo->y);
	if (my->isBlinking)
		sySpriteDraw(.sprite = &my->spriteBlink, .x = ogmo->x, .y = ogmo->y);
	if (my->isTalking)
		sySpriteDraw(.sprite = &my->spriteTalk, .x = ogmo->x, .y = ogmo->y);
	
	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objNpcClass = {
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

