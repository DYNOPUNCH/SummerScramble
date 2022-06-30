/* 
 * objNpc.c <SummerScramble>
 * 
 * the primary NPC object
 * 
 */

#include <objNpc.h>
#include <objNpcHooks.h>

/* <ogmodefaults> */
const struct objNpc objNpcDefaults = 
{
	/* <ogmoblock> */
	.State = 0
	, .unused___ = 0
	/* </ogmoblock> */
	
	/* <ogmoblock1> */
	/* defaults for custom user-defined variables go here */
	/* </ogmoblock1> */
};
/* </ogmodefaults> */

/* private functions */

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
	
	/* randomized blinking between cycles */
	if ((--owner->blinkCountDown) <= 0)
	{
		owner->blinkCountDown = rand() % 3 + 3;
		owner->isBlinking = true;
	}
	else
		owner->isBlinking = false;
}

static void SetupFrames(struct objNpc *my, const char *StateStr)
{
	const char *stateLUT[] =
	{
		[objNpcState_Main] = "Main"
		, [objNpcState_COUNT] = 0
	};
	
	/* default to 'Main' if given state doesn't exist */
	if (!StateStr && !(StateStr = stateLUT[my->State]))
		StateStr = "Main";
	
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
	float x = my->x;
	float y = my->y;
	
	ezrect(x, y, 16, 16, -1);
	
	sySpriteDraw(.sprite = &my->spriteBody, .x = x, .y = y);
	if (my->isBlinking)
		sySpriteDraw(.sprite = &my->spriteBlink, .x = x, .y = y);
	if (my->isTalking)
		sySpriteDraw(.sprite = &my->spriteTalk, .x = x, .y = y);
	
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

