/* 
 * objExample.h <ProjectName>
 * 
 * your description here
 * 
 */

#ifndef objExample_H_INCLUDED
#define objExample_H_INCLUDED

#include "common-includes.h"

/* <ogmoenums> */
enum objExamplemyEnum
{
	objExamplemyEnum_a
	, objExamplemyEnum_b
	, objExamplemyEnum_c
	, objExamplemyEnum_c1
};
/* </ogmoenums> */

/* <ogmostruct> */
struct objExample
{
	/* <ogmoblock> */
	bool myBoolean;
	unsigned int myColor;
	enum objExamplemyEnum myEnum;
	const char *myFilepath;
	float myFloat;
	int myInteger;
	const char *myString;
	const char *myText;
	bool unused___;
	/* </ogmoblock> */
	
	/* custom user-defined variables go here */
};
/* <ogmoblock1> */
extern const struct objExample objExampleDefaults;
extern const struct syOgmoEntityClass objExampleClass;
#define objExampleValues(...) (struct objExample){.myBoolean = false, .myColor = 0x000000ff, .myEnum = 0, .myFilepath = "proj:", .myFloat = 0, .myInteger = 0, .myString = "", .myText = "", .unused___ = 0,  __VA_ARGS__ }
/* </ogmoblock1> */
/* </ogmostruct> */

#endif /* objExample_H_INCLUDED */

