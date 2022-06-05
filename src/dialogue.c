/*
 * dialogue.c <SummerScramble>
 *
 * conversation processing
 */

#include "syText.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define KEY_isEnd       "isEnd"
#define KEY_isQuestion  "isQuestion"
#define KEY_option      "option"
#define KEY_goto        "goto"
#define KEY_character   "character"
#define KEY_emotion     "emotion"

struct DialogueOption
{
	const char *text;
	const char *label;
};

struct Dialogue
{
	const struct syText *text;
	const char *character; /* who is speaking */
	const char *emotion; /* how they feel */
	struct DialogueOption *option;
	int optionNum;
	bool isEnd;
	bool isQuestion;
};

/* set up Dialogue based on syText contents */
static void MakeFromSyText(struct Dialogue *dst, const struct syText *src)
{
	int gotoNum;
	assert(dst);
	assert(src);
	
	memset(dst, 0, sizeof(*dst));
	
	/* misc parameters */
	dst->text = src;
	dst->character = syTextGetUdataColumnValue(src, KEY_character);
	dst->emotion = syTextGetUdataColumnValue(src, KEY_emotion);
	dst->isEnd = syTextGetUdataColumnValue(src, KEY_isEnd) != 0;
	dst->isQuestion = syTextGetUdataColumnValue(src, KEY_isQuestion) != 0;
	
	/* handle jumps */
	gotoNum = syTextGetUdataColumnValueCount(src, KEY_goto);
	if (gotoNum)
	{
		int i;
		
		dst->option = calloc(gotoNum, sizeof(*(dst->option)));
		dst->optionNum = gotoNum;
		assert(dst->option);
		
		for (i = 0; i < gotoNum; ++i)
		{
			struct DialogueOption *op = &dst->option[i];
			
			op->text = syTextGetUdataColumnValueRow(src, KEY_option, i);
			op->label = syTextGetUdataColumnValueRow(src, KEY_goto, i);
		}
	}
}

bool DialogueFinished(const struct Dialogue *d)
{
	assert(d);
	
	return d->text == 0;
}

void DialogueDisplay(struct Dialogue *d)
{
	const struct syText *m = d->text;
	
	fprintf(stderr, "[%s] %s\n", syTextGetLabel(m), syTextGetContents(m));
	if (d->isQuestion)
	{
		for (int i = 0; i < d->optionNum; ++i)
			fprintf(stderr, " %d : %s\n", i, d->option[i].text);
		fprintf(stderr, "(choose one)\n");
	}
	else
	{
		fprintf(stderr, "(press enter)\n");
	}
}

void DialogueStart(struct Dialogue *d, const char *label)
{
	const struct syText *text = syTextFindByLabel(label);
	
	MakeFromSyText(d, text);
}

void DialogueAdvance(struct Dialogue *d, int choice)
{
	assert(d);
	
	/* multiple possible selections */
	if (d->isQuestion)
	{
		assert(choice >= 0 && choice < d->optionNum);
		
		if (choice < 0 || choice >= d->optionNum)
		{
			fprintf(
				stderr
				, "message (label='%s'), invalid choice='%d'"
				, syTextGetLabel(d->text)
				, choice
			);
			abort();
			return;
		}
		DialogueStart(d, d->option[choice].label);
	}
	/* last message */
	else if (d->isEnd)
	{
		d->text = 0;
	}
	/* regular goto */
	else if (d->optionNum)
	{
		DialogueStart(d, d->option[0].label);
	}
	/* advance to next "row" */
	else
	{
		MakeFromSyText(d, syTextNextInArray(d->text));
	}
}

struct Dialogue *DialogueNew(void)
{
	struct Dialogue *r;
	
	r = calloc(1, sizeof(*r));
	assert(r);
	
	return r;
}

void DialogueDelete(struct Dialogue *v)
{
	free(v);
}

void DialogueDump(struct Dialogue *d, FILE *out)
{
	int i;
	
	assert(d);
	assert(out);
	
	//syTextDump(d->text);
	for (i = 0; i < d->optionNum; ++i)
		fprintf(out, "'%s' goto(%s)\n", d->option[i].text, d->option[i].label);
}

