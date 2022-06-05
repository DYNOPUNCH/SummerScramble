/* 
 * objGoToRoom.h <ProjectName>
 * 
 * your description here
 * 
 */

#ifndef objGoToRoom_H_INCLUDED
#define objGoToRoom_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
/* </ogmoenums> */

/* <ogmostruct> */
struct objGoToRoom
{
	/* <ogmoblock> */
	const char *room;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
};
/* <ogmoblock1> */
extern const struct objGoToRoom objGoToRoomDefaults;
extern const struct syOgmoEntityClass objGoToRoomClass;
#define objGoToRoomValues(...) (struct objGoToRoom){.room = "test", .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objGoToRoom_H_INCLUDED */

