/* 
 * objExample.c <ProjectName>
 * 
 * your description here
 * 
 */

#include <objExample.h>

/* <ogmodefaults> */
const struct objExample objExampleDefaults = 
{
	/* <ogmoblock> */
	.myBoolean = false
	, .myColor = 0x000000ff
	, .myEnum = 0
	, .myFilepath = "proj:"
	, .myFloat = 0
	, .myInteger = 0
	, .myString = ""
	, .myText = ""
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
	struct objExample *my = MALLOC(sizeof(*my));
	
	assert(my);
	
	if (!src)
		src = &objExampleDefaults;
	
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

/* <ogmoclass> */
const struct syOgmoEntityClass objExampleClass = {
	.New = New
	, .funcs = (syOgmoEntityFunc[]){
		[syOgmoExec_Draw] = Draw
	}
	, .funcsCount = sizeof((char[]){
		[syOgmoExec_Draw] = 0
	}) / sizeof(char)
};
/* </ogmoclass> */

