/* 
 * objConversation.c <SummaaEsukaramuburu>
 * 
 * a conversation involving one or more characters
 * 
 */

#include <objConversation.h>

/* <ogmodefaults> */
const struct objConversation objConversationDefaults = 
{
	/* <ogmoblock> */
	.Label = "unset"
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
	struct objConversation *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objConversationDefaults;
	
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
	#include "debug_draw.h"

	return 0;
}

syOgmoEntityFuncDecl(Init)
{
	struct objConversation *my = ogmo->values;
	struct Dialogue *d = &my->Dialogue;
	
	syTextSetTable("Conversations");
	
	debug("init objConversation where Label='%s'\n", my->Label);
	
	DialogueStart(d, my->Label);
	while (!DialogueFinished(d))
	{
		int choice;
		
		DialogueDisplay(d);
		choice = getchar();
		DialogueAdvance(d, choice - '0');
		fflush(stdin);
	}
	
	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objConversationClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Draw] = Draw
		, [syOgmoExec_Init] = Init
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Draw] = 0
		, [syOgmoExec_Init] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

