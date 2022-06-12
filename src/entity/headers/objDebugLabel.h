/* 
 * objDebugLabel.h <ProjectName>
 * 
 * your description here
 * 
 */

#ifndef objDebugLabel_H_INCLUDED
#define objDebugLabel_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
/* </ogmoenums> */

/* <ogmostruct> */
struct objDebugLabel
{
	/* <ogmoblock> */
	const char *text;
	unsigned int color;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
	struct sySprite sprite;
};
/* <ogmoblock1> */
extern const struct objDebugLabel objDebugLabelDefaults;
extern const struct syOgmoEntityClass objDebugLabelClass;
#define objDebugLabelValues(...) (struct objDebugLabel){.text = "", .color = 0x000000ff, .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objDebugLabel_H_INCLUDED */

