/* 
 * objConversation.c <SummaaEsukaramuburu>
 * 
 * a conversation involving one or more characters
 * 
 */

#include <objConversation.h>
#include <objNpc.h>

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
	struct objConversation *my = ogmo->values;
	struct Dialogue *d = &my->Dialogue;
	
	if (!DialogueFinished(d))
	{
		if (my->whoPtr != d->character && strcmp(my->who, d->character))
		{
			strcpy(my->who, d->character);
			my->whoPtr = d->character;
			fprintf(stderr, "%s\n", d->character);
			//my->Npc[0] = syOgmo
			my->NpcInst = syOgmoEntityNewWith(objNpc, .Name = my->who, .x = 480 / 2, .y = 270);
			my->Npc[0] = my->NpcInst->values;
		}
		
		{
			const struct syText *m = d->text;
			const char *contents = syTextGetContents(m);
			
			my->Npc[0]->isTalking = strlen(contents) > my->typewriter;
			
			if (DebugButton(
					0
					, 270 - 270 / 4
					, 480
					, 270 / 4
					, 0xffffffcc
					, "%s:\n""%.*s"
					, d->character
					, (int)my->typewriter
					, contents
				) == MouseState_Clicked
			)
			{
				DialogueAdvance(d, 0);
				my->typewriter = 0;
			}
			
			// reference code from DialogueDisplay():
			/*fprintf(stderr, "[%s] %s\n", syTextGetLabel(m), syTextGetContents(m));
			if (d->isQuestion)
			{
				for (int i = 0; i < d->optionNum; ++i)
					fprintf(stderr, " %d : %s\n", i, d->option[i].text);
				fprintf(stderr, "(choose one)\n");
			}
			else
			{
				fprintf(stderr, "(press enter)\n");
			}*/
		}
	}
	
	else if (my->NpcInst)
	{
		syOgmoEntityDelete(my->NpcInst);
		my->NpcInst = 0;
	}
	
	while (false && !DialogueFinished(d))
	{
		int choice;
		
		DialogueDisplay(d);
		choice = getchar();
		DialogueAdvance(d, choice - '0');
		fflush(stdin);
	}

	return 0;
}

syOgmoEntityFuncDecl(Step)
{
	struct objConversation *my = ogmo->values;
	
	my->typewriter += 10 / 60.0;
	
	return 0;
}

syOgmoEntityFuncDecl(Init)
{
	struct objConversation *my = ogmo->values;
	struct Dialogue *d = &my->Dialogue;
	
	syTextSetTable("Conversations");
	
	debug("init objConversation where Label='%s'\n", my->Label);
	
	DialogueStart(d, my->Label);
	
	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objConversationClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Draw] = Draw
		, [syOgmoExec_Step] = Step
		, [syOgmoExec_Init] = Init
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Draw] = 0
		, [syOgmoExec_Step] = 0
		, [syOgmoExec_Init] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

