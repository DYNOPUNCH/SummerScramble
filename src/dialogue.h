/*
 * dialogue.c <SummerScramble>
 *
 * conversation processing
 */

#ifndef SUMMER_DIALOGUE_H_INCLUDED
#define SUMMER_DIALOGUE_H_INCLUDED

#include <stdbool.h>
#include <syText.h>

struct DialogueOption
{
	const char *text;
	const char *label;
};

struct Dialogue
{
	const struct syText *text;
	const char *character; /* who is speaking */
	const char *portrait; /* portrait to display */
	const char *emotion; /* how they feel */
	struct DialogueOption *option;
	int optionNum;
	bool isEnd;
	bool isQuestion;
};

/* public functions */
struct Dialogue *DialogueNew(void);
void DialogueDelete(struct Dialogue *v);
void DialogueStart(struct Dialogue *d, const char *label);
void DialogueDisplay(struct Dialogue *d);
void DialogueAdvance(struct Dialogue *d, int choice);
void DialogueDump(struct Dialogue *d, FILE *out);
bool DialogueFinished(const struct Dialogue *d);

#endif /* SUMMER_DIALOGUE_H_INCLUDED */

