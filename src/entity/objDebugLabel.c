/* 
 * objDebugLabel.c <ProjectName>
 * 
 * your description here
 * 
 */

#include <objDebugLabel.h>

/* <ogmodefaults> */
const struct objDebugLabel objDebugLabelDefaults = 
{
	/* <ogmoblock> */
	.text = ""
	, .color = 0x000000ff
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
	struct objDebugLabel *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objDebugLabelDefaults;
	
	memcpy(my, src, sizeof(*my));
	/* </ogmoblock> */
	
	/* custom code goes here */
	
	/* <ogmoblock1> */
	return my;
	/* </ogmoblock1> */
}
/* </ogmonew> */

syOgmoEntityFuncDecl(Draw)
{
	struct objDebugLabel *my = ogmo->values;
	
	DebugLabel(ogmo->x, ogmo->y, ogmo->width, ogmo->height, my->color, my->text);
	
	/* init (placeholder) */
	if (!my->sprite.owner)
	{
		sySpriteInit(&my->sprite, my, 0);
		sySpriteSetAnimation(&my->sprite, "Swordsman");
	}
	
	/* step (placeholder) */
	my->sprite.ms_scaled += 16.666;
	sySpriteStep(&my->sprite);
	
	/* draw */
	sySpriteDraw(
		.sprite = &my->sprite
		, .x = ogmo->x
		, .y = ogmo->y
	);
	sySpriteDraw(
		.sprite = &my->sprite
		, .x = ogmo->x + ogmo->width
		, .y = ogmo->y + ogmo->height
		, .xscale = -1
		, .yscale = -1
	);

	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objDebugLabelClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Draw] = Draw
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Draw] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

