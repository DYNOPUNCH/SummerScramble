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

static int FuncSortNpcStack(const void *a_, const void *b_)
{
	const struct syOgmoEntity *a = *(const struct syOgmoEntity**)a_;
	const struct syOgmoEntity *b = *(const struct syOgmoEntity**)b_;
	const struct objNpc *av;
	const struct objNpc *bv;
	
	if (!a || !(av = a->values))
		return 1;
	
	if (!b || !(bv = b->values))
		return -1;
	
	/*debug("a vs b : %p vs %p\n", a, b);
	debug("av  bv : %p vs %p\n", av, bv);
	debug("%d\n", av->stackOrder - bv->stackOrder);*/
	
	return av->stackOrder - bv->stackOrder;
}

static void SortNpcStack(struct objConversation *my)
{
	/* sort the stack */
	//for (int i = 0; i < NPC_STACK_MAX; ++i)
	//	debug("[p%d] = %p : %p\n", i, my->NpcInst[i], my->NpcInst[i] ? my->NpcInst[i]->values : 0);
	qsort(my->NpcInst, NPC_STACK_MAX, sizeof(*my->NpcInst), FuncSortNpcStack);
	
	/* refresh handles for each */
	for (int i = 0; i < NPC_STACK_MAX; ++i)
	{
		my->Npc[i] = 0;
		
		if (my->NpcInst[i])
			my->Npc[i] = my->NpcInst[i]->values;
	}
}

syOgmoEntityFuncDecl(Draw)
{
	struct objConversation *my = ogmo->values;
	struct Dialogue *d = &my->Dialogue;
	
	if (!DialogueFinished(d))
	{
		/* if the string representing who is speaking has changed */
		if (my->whoPtr != d->character && strcmp(my->who, d->character))
		{
			bool matchFound = false;
			
			strcpy(my->who, d->character);
			my->whoPtr = d->character;
			fprintf(stderr, "%s\n", d->character);
			//my->Npc[0] = syOgmo
			
			// TODO allow explicitly deleting NPCs from the stack during a conversation
			// (for example, one NPC leaves in the middle of a three-way conversation)
			
			/* check whether NPC already exists within stack */
			for (int i = 0; i < NPC_STACK_MAX; ++i)
			{
				struct objNpc *walk = my->Npc[i];
				
				if (!walk)
					continue;
				
				walk->stackOrder += 1;
				
				/* if it does, queue it back to the beginning */
				if (matchFound == false && !strcmp(walk->Name, my->who))
				{
					matchFound = true;
					walk->stackOrder = 0;
				}
			}
			
			/* update stack order in case any have shifted index */
			SortNpcStack(my);
			
			/* no match found in stack */
			if (matchFound == false)
			{
				/* overwrite whichever has been in the stack the longest (expired) */
				my->NpcInst[NPC_STACK_MAX - 1] = syOgmoEntityNewWith(objNpc, .Name = my->who, .x = 480 / 2, .y = 270);
				
				/* move it back to the beginning */
				SortNpcStack(my);
			
				/*for (int k = 0; k < NPC_STACK_MAX; ++k)
					debug("[%d] = %p\n", k, my->NpcInst[k]);*/
			}
		}
		
		{
			const struct syText *m = d->text;
			const char *contents = syTextGetContents(m);
			
			/* stack is always ordered such that the speaker is the first */
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
	
	else if (true)
	{
		for (int i = 0; i < NPC_STACK_MAX; ++i)
		{
			if (!my->NpcInst[i])
				continue;
			syOgmoEntityDelete(my->NpcInst[i]);
			my->NpcInst[i] = 0;
		}
		
		/* TODO delete self */
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

