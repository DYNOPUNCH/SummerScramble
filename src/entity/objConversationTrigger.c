/* 
 * objConversationTrigger.c <ProjectName>
 * 
 * your description here
 * 
 */

#include <objConversationTrigger.h>
#include <objConversationTriggerHooks.h>

#include <objConversation.h>

/* <ogmodefaults> */
const struct objConversationTrigger objConversationTriggerDefaults = 
{
	/* <ogmoblock> */
	.Label = "unset"
	, .OnClick = __objConversationTriggerUniqueFunc0
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
	struct objConversationTrigger *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objConversationTriggerDefaults;
	
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
	struct objConversationTrigger *my = ogmo->values;
	
	//ezrect(ogmo->x, ogmo->y, ogmo->width, ogmo->height, -1);
	
	if (DebugButton(
			ogmo->x
			, ogmo->y
			, ogmo->width
			, ogmo->height
			, COLOR_PINK
			, "%p"
			, my
		) == MouseState_Clicked
	)
	{
		if (my->OnClick)
			my->OnClick(syOgmoEntityFuncShareArgs);
		
		debug("clicked objConversationTrigger where Label='%s'\n", my->Label);
		syOgmoEntityNewWith(objConversation, .Label = my->Label);
		//QueueRoom(my->room);
	}

	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objConversationTriggerClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Draw] = Draw
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Draw] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

