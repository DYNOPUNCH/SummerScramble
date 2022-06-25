/* 
 * objConversationTrigger.h <ProjectName>
 * 
 * your description here
 * 
 */

#ifndef objConversationTrigger_H_INCLUDED
#define objConversationTrigger_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
/* </ogmoenums> */

/* <ogmostruct> */
struct objConversationTrigger
{
	/* <ogmoblock> */
	const char *Label;
	syOgmoEntityFunc OnClick;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
};
/* <ogmoblock1> */
extern const struct objConversationTrigger objConversationTriggerDefaults;
extern const struct syOgmoEntityClass objConversationTriggerClass;
#define objConversationTriggerValues(...) (struct objConversationTrigger){.Label = "unset", .OnClick = __objConversationTriggerUniqueFunc0, .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objConversationTrigger_H_INCLUDED */

