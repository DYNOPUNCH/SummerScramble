/*
 * syTsv.h <SyrupEngine>
 *
 * tsv (tab separated values) spreadsheet parser
 */

#ifndef SYRUP_TSV_H_INCLUDED
#define SYRUP_TSV_H_INCLUDED

#include <stdbool.h>

/*
 *
 * opaque types
 *
 */
struct syTsv;

/*
 *
 * public functions
 *
 */

/* allocates an internal TSV structure */
struct syTsv *syTsvNew(void);

/* frees an internal syTsv structure and its private members */
void syTsvFree(struct syTsv *tsv);

/* returns the string stored in cell[row, column] within the TSV
 * (returns 0 if the cell is blank)
 */
char *syTsvGet(struct syTsv *tsv, int row, int column);

/* get number of rows */
int syTsvGetNumRows(struct syTsv *tsv);

/* get number of columns */
int syTsvGetNumColumns(struct syTsv *tsv);

/* query whether row is blank */
bool syTsvIsRowBlank(struct syTsv *tsv, int row);

/* searches row 'row' of the TSV for a column matching 'name'
 * (produces code easier to read and maintain than hard-coded column indices)
 */
int syTsvColumnIndex(struct syTsv *tsv, int row, const char *name);
#define syTsvHeaderIndex(TSV, NAME) syTsvColumnIndex(TSV, 0, NAME)

/* XXX
 * performs destructive operations on tsvDataString and takes
 * responsibility for freeing it, so don't free it yourself!
 */
void syTsvParse(struct syTsv *tsv, const char *id, char *tsvDataString);

/* copies the data before modifying it, so
 * it's safe to use on read-only variables
 */
void syTsvParseConst(struct syTsv *tsv, const char *id, const char *tsvDataString);

#endif /* SYRUP_TSV_H_INCLUDED */

