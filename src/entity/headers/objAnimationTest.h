/* 
 * objAnimationTest.h <ProjectName>
 * 
 * your description here
 * 
 */

#ifndef objAnimationTest_H_INCLUDED
#define objAnimationTest_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
/* </ogmoenums> */

/* <ogmostruct> */
struct objAnimationTest
{
	/* <ogmoblock> */
	bool new_value;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
	struct sySprite spriteBase;
	struct sySprite spriteEyes;
	struct sySprite spriteMouth;
	unsigned blinkStart;
};
/* <ogmoblock1> */
extern const struct objAnimationTest objAnimationTestDefaults;
extern const struct syOgmoEntityClass objAnimationTestClass;
#define objAnimationTestValues(...) (struct objAnimationTest){.new_value = false, .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objAnimationTest_H_INCLUDED */

