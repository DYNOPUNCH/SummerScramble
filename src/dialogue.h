/*
 * dialogue.c <SummerScramble>
 *
 * conversation processing
 */

#ifndef SUMMER_DIALOGUE_H_INCLUDED
#define SUMMER_DIALOGUE_H_INCLUDED

#include <stdbool.h>

/* opaque types */
struct Dialogue;

/* public functions */
struct Dialogue *DialogueNew(void);
void DialogueDelete(struct Dialogue *v);
void DialogueStart(struct Dialogue *d, const char *label);
void DialogueDisplay(struct Dialogue *d);
void DialogueAdvance(struct Dialogue *d, int choice);
void DialogueDump(struct Dialogue *d, FILE *out);
bool DialogueFinished(const struct Dialogue *d);

#endif /* SUMMER_DIALOGUE_H_INCLUDED */

