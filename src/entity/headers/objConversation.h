/* 
 * objConversation.h <SummaaEsukaramuburu>
 * 
 * a conversation involving one or more characters
 * 
 */

#ifndef objConversation_H_INCLUDED
#define objConversation_H_INCLUDED

#include <objNpc.h>

#include "common-includes.h"

/* <ogmoenums> */
/* </ogmoenums> */

#define NPC_STACK_MAX  4

/* <ogmostruct> */
struct objConversation
{
	/* <ogmoblock> */
	const char *Label;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
	const char *whoPtr;
	char who[32];
	struct Dialogue Dialogue;
	struct objNpc *Npc[NPC_STACK_MAX];
	struct syOgmoEntity *NpcInst[NPC_STACK_MAX];
	float typewriter;
};
/* <ogmoblock1> */
extern const struct objConversation objConversationDefaults;
extern const struct syOgmoEntityClass objConversationClass;
#define objConversationValues(...) (struct objConversation){.Label = "unset", .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objConversation_H_INCLUDED */

