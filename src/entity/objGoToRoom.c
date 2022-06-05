/* 
 * objGoToRoom.c <ProjectName>
 * 
 * your description here
 * 
 */

#include <objGoToRoom.h>

/* <ogmodefaults> */
const struct objGoToRoom objGoToRoomDefaults = 
{
	/* <ogmoblock> */
	.room = "test"
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
	struct objGoToRoom *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objGoToRoomDefaults;
	
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
	struct objGoToRoom *my = ogmo->values;
	
	if (DebugButton(
			ogmo->x
			, ogmo->y
			, ogmo->width
			, ogmo->height
			, COLOR_PINK
			, "%s"
			, my->room
		) == MouseState_Clicked
	)
	{
		debug("clicked '%s'\n", my->room);
		QueueRoom(my->room);
	}

	return 0;
}

/* <ogmoclass> */
const struct syOgmoEntityClass objGoToRoomClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Draw] = Draw
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Draw] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

