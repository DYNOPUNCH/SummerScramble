/* 
 * objConversation.h <SummaaEsukaramuburu>
 * 
 * a conversation involving one or more characters
 * 
 */

#ifndef objConversation_H_INCLUDED
#define objConversation_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
/* </ogmoenums> */

/* <ogmostruct> */
struct objConversation
{
	/* <ogmoblock> */
	const char *Label;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
	struct Dialogue Dialogue;
};
/* <ogmoblock1> */
extern const struct objConversation objConversationDefaults;
extern const struct syOgmoEntityClass objConversationClass;
#define objConversationValues(...) (struct objConversation){.Label = "unset", .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objConversation_H_INCLUDED */

