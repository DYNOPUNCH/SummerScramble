/*
 * syText.h <SyrupEngine>
 *
 * for parsing and querying Syrup text databases
 *
 * see syText.c for text database structure
 */

#ifndef SYRUP_TEXT_H_INCLUDED
#define SYRUP_TEXT_H_INCLUDED

/* opaque types */
struct syText;
struct syTextTable;
struct syTextMachine;

/* public functions */
void syTextInit(void);
void syTextCleanup(void);
const struct syText *syTextFindByLabel(const char *label);
void syTextAddLocale(const char *name, const char *defaultFont, const char *tsvContents);
void syTextSetLocale(const char *name);
void syTextAddFont(const char *name, const void *ttfData, const unsigned ttfFilesize, int defaultHeight);
void syTextUseFont(const char *name);
const char *syTextGetUdataColumnValue(const struct syText *text, const char *name);
const char *syTextGetUdataColumnValueSafe(const struct syText *text, const char *name);
int syTextGetUdataColumnValueCount(const struct syText *text, const char *name);
const char *syTextGetUdataColumnValueRow(const struct syText *text, const char *name, int row);
const char *syTextGetLabel(const struct syText *t);
const char *syTextGetContents(const struct syText *t);
const struct syText *syTextNextInArray(const struct syText *t);
void syTextSetTable(const char *name);
const char *syTextGetTable(void);

/* testing */
void syTextDumpLocale(const char *name, const char *outfn);

#endif /* SYRUP_TEXT_H_INCLUDED */

