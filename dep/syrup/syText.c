/*
 * syText.c <SyrupEngine>
 *
 * for parsing and querying Syrup text databases
 *
 * text databases are TSV-formatted spreadsheets
 *
 * data is stored like so:
 * - Machine
 *    - Locale array
 *       - Text table array
 *          - Table name
 *          - Table column names
 *          - Message array
 *             - Label
 *             - Associated text
 *             - udata[udataH][udataW] (row major access aka [row][col])
 * 
 * a simple spreadsheet looks something like this:
 * (the first row is for reference only)
 * | column 0 | column 1    |    ...     | column n        |
 * | ~        | menus       | name udata |                 |
 * | label    | text        | userdata   |                 |
 * | label    | text        | userdata   |                 |
 * |          | fallthrough | userdata   |                 |
 * | etc      | etc         | etc        |                 |
 * | ~        | dialogue    | name udata | name more udata |
 * | label    | text        | etc        | etc             |
 * |          |             | extended   | udata           |
 * 
 * declaring tables:
 * - to declare a table, the first cell in a row should be '~'
 * - the next cell is the table's name (must be unique)
 * - all cells that follow are for defining the table's header
 *   (extra fields specific to the table are named here)
 *    * an empty cell specifies the end of the table's header
 * - one table ends where the next table begins
 *   (or at the end of the file)
 *
 * declaring labels:
 * - the first cell in a row is a label's name (must be unique)
 * - the next cell is the string to associate with that label
 * - all cells that follow are userdata corresponding to any
 *   extra fields that the table might have defined in its header
 *    * a fatal error is thrown for cells in columns that fall
 *      outside the table's boundaries
 *      (XXX actually, such cells are currently silently ignored)
 *    * udata can span multiple rows
 *    * udata ends when a new label, label-less string, table, or
 *      empty row is encountered
 * - labels can have multiple strings attached; index 0 refers to the
 *   current label, and indices [1], [...], [n] can be used to access
 *   the label-less strings on the rows that follow
 *
 * extra info:
 * - empty rows are ignored
 * - empty cells are accessible, but point to zero
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "syTsv.h"

static struct syTextMachine *g = 0;

/*
 * private types and functions
 */
struct syTextKey
{
	const char *string;
	int index;
};

struct syTextOption
{
	const char *string;
	struct syTextKey key;
};

struct syTextFont
{
	char *name;
	void *data;
	struct syTextFont *next;
};

struct syText
{
	struct syTextKey key;
	const char *contents;
	const struct syTextTable *table; /* parent */
	char ***udata;
	int udataW;
	int udataH;
};

struct syTextTable
{
	const char *name;
	struct syText *msg;
	int msgNum;
	const char **header;
	int headerNum;
};

struct syTextLocale
{
	const char *name;
	struct syTextTable *table;
	struct syTextFont *font;
	struct syTsv *tsv;
	int tableNum;
};

struct syTextMachine
{
	struct syTextLocale *locale;
	struct syTextTable *table;
	int localeNum;
};

#if 0
static void syTextTest(void)
{
	const char *wow[8][4] = {
		{ "a", "b", "c", "d" }
		, { "e", "f", "g", "h" }
		, { "i", "j", "k", "l" }
		, { "m", "n", "o", "p" }
		, { "q", "r", "s", "t" }
		, { "u", "v", "w", "x" }
		, { "y", "z", "0", "1" }
		, { "2", "3", "4", "5" }
	};
	struct syText t = {0};
	
	/* init and populate */
	t.udataW = 4;
	t.udataH = 8;
	t.udata = malloc(t.udataH * sizeof(char*));
	for (int i = 0; i < t.udataH; ++i)
	{
		t.udata[i] = malloc(t.udataW * sizeof(char*));
		for (int k = 0; k < t.udataW; ++k)
			t.udata[i][k] = wow[i][k];
	}
	
	/* print */
	for (int col = 0; col < t.udataH; ++col)
		for (int row = 0; row < t.udataW; ++row)
			fprintf(stderr, "[%d][%d] = '%s'\n", col, row, t.udata[col][row]);
	
	/* cleanup */
	for (int col = 0; col < t.udataH; ++col)
		free(t.udata[col]);
	free(t.udata);
	
	exit(EXIT_SUCCESS);
}
#endif

#if 0
static void syTextKeyFree(struct syTextKey *k)
{
	assert(k);
	
	if (k->string)
		free(k->string);
}

static void syTextFree(struct syText *t)
{
	assert(t);
	
	syTextKeyFree(&t->key);
	free(t->contents);
	free(t);
}

static void syTableFree(struct syTextTable *t)
{
	assert(t);
	
	/* TODO */
}

static void syTextLocaleFree(struct syTextLocale *l)
{
	assert(l);
	assert(l->name);
	
	free(l->name);
	
	/* messages */
	{
		struct syTextTable *t;
		
		for (t = l->table; t < l->table + l->tableNum; ++t)
			syTableFree(t);
	}
	
	/* TODO fonts */
	
	free(l);
}

static void syTextMachineFree(struct syTextMachine *d)
{
	assert(d);
	
	/* locales */
	{
		struct syTextLocale *l;
		
		for (l = d->locale; l < d->locale + d->localeNum; ++l)
			syTextLocaleFree(l);
	}
	
	free(d);
}

static struct syText *syTextFindByIndex(int index)
{
	if (index < 0 || index >= g->table->msgNum)
		return 0;
	
	return g->table->msg + index;
}
#endif

static struct syTextLocale *FindLocale(const char *name)
{
	struct syTextLocale *l;
	
	for (l = g->locale; l < g->locale + g->localeNum; ++l)
		if (!strcmp(l->name, name))
			return l;
	
	return 0;
}

static struct syTextTable *FindTable(const char *name)
{
	struct syTextTable *t;
	struct syTextLocale *l;
	
	if (!g || !g->locale)
		return 0;
	
	/* search current locale for table of matching name */
	l = g->locale;
	for (t = l->table; t < l->table + l->tableNum; ++t)
		if (!strcmp(t->name, name))
			return t;
	
	return 0;
}

/*
 * public functions
 */

void syTextSetLocale(const char *name)
{
	g->locale = FindLocale(name);
	
	if (!g->locale)
	{
		fprintf(stderr, "unknown locale '%s'\n", name);
		exit(EXIT_FAILURE);
	}
	
	/* if there was a table used by the old locale, try finding it */
	if (g->table)
	{
		g->table = FindTable(g->table->name);
		assert(g->table);
	}
}

void syTextSetTable(const char *name)
{
	assert(g);
	
	g->table = FindTable(name);
}

const struct syText *syTextFindByLabel(const char *label)
{
	struct syText *m;
	
	if (!label)
		return 0;
	
	assert(g);
	assert(g->table);
	
	for (m = g->table->msg; m < g->table->msg + g->table->msgNum; ++m)
		if (m->key.string && !strcmp(m->key.string, label))
			return m;
	
	return 0;
}

/* returns column index >= 0 matching key in udata table
 * (returns -1 if no column name matching key is found) */
static int UdataKeyColumnIndex(const struct syText *text, const char *key)
{
	const struct syTextTable *table;
	int column;
	
	assert(text);
	assert(key);
	
	/* if udata is empty or doesn't contain
	 * enough rows to store key/value pairs
	 */
	if (!text->udata || text->udataH < 1 || !text->table)
		return -1;
	
	/* search for match */
	table = text->table;
	for (column = 0; column < table->headerNum; ++column)
		if (table->header[column] && !strcmp(table->header[column], key))
			break;
	
	/* no match */
	if (column == table->headerNum)
		return -1;
	
	return column;
}

/* given the name of a udata column, returns the first value within it,
 * or zero (0) if either (a) no key exists, or (b) the column is empty */
const char *syTextGetUdataColumnValue(const struct syText *text, const char *name)
{
	int column = UdataKeyColumnIndex(text, name);
	
	/* no match found */
	if (column < 0)
		return 0;
	
	/* return cell on first row */
	return text->udata[0][column];
}

/* same as syTextGetUdataColumnValue, but returns empty string instead of 0 */
const char *syTextGetUdataColumnValueSafe(const struct syText *text, const char *name)
{
	const char *r = syTextGetUdataColumnValue(text, name);
	
	if (!r)
		return "";
	
	return r;
}

/* returns the number of values stored in a given column */
int syTextGetUdataColumnValueCount(const struct syText *text, const char *name)
{
	int row;
	int count = 0;
	int column = UdataKeyColumnIndex(text, name);
	
	/* no match found */
	if (column < 0)
		return 0;
	
	/* count rows that have a value in this column */
	for (row = 0; row < text->udataH; ++row, ++count)
		if (!text->udata[row][column])
			break;
	
	return count;
}

/* get a udata value at a specific row in a named column
 * (returns 0 if out of bounds or empty)
 */
const char *syTextGetUdataColumnValueRow(const struct syText *text, const char *name, int row)
{
	int column = UdataKeyColumnIndex(text, name);
	
	/* no match found */
	if (column < 0)
		return 0;
	
	/* out of bounds */
	if (row >= text->udataH)
		return 0;
	
	return text->udata[row][column];
}

void syTextAddLocale(const char *name, const char *defaultFont, const char *tsvString)
{
	struct syTsv *tsv;
	struct syTextLocale *loc;
	struct syTextTable *table;
	struct syText *text = 0;
	int row;
	int col;
	int rowNum;
	int colNum;
	
	assert(name);
	assert(defaultFont);
	assert(tsvString);
	
	if (FindLocale(name))
	{
		fprintf(stderr, "locale '%s' already exists, can't re-add it\n", name);
		exit(EXIT_FAILURE);
	}
	
	/* create new zero-initialized locale in list and get pointer to it */
	g->locale = realloc(g->locale, sizeof(*(g->locale)) * (g->localeNum + 1));
	assert(g->locale);
	loc = g->locale + g->localeNum;
	g->localeNum += 1;
	memset(loc, 0, sizeof(*loc));
	
	/* initialize locale */
	loc->name = name;
	
	/* within the locale, initialize the tsv */
	loc->tsv = syTsvNew();
	tsv = loc->tsv;
	assert(tsv);
	syTsvParseConst(tsv, tsvString, tsvString);
	
	/* get some info about it */
	rowNum = syTsvGetNumRows(tsv);
	colNum = syTsvGetNumColumns(tsv);
	assert(rowNum >= 2);
	assert(colNum >= 2); /* the minimum */
	
	/* TODO parse rows/columns into array of table, array of message */
	
	/* first pass through spreadsheet: count tables */
	for (loc->tableNum = 0, row = 0; row < rowNum; ++row)
	{
		const char *first;
		
		first = syTsvGet(tsv, row, 0);
		
		/* skip blank rows */
		if (!first)
			continue;
		
		/* table declarator */
		if (!strcmp(first, "~"))
			loc->tableNum += 1;
	}
	
	/* allocate tables */
	assert(loc->tableNum);
	loc->table = calloc(loc->tableNum, sizeof(*(loc->table)));
	assert(loc->table);
	
	/* second pass through spreadsheet: count messages in each table */
	for (table = loc->table - 1, row = 0; row < rowNum; ++row)
	{
		const char *first;
		const char *second;
		
		first = syTsvGet(tsv, row, 0);
		second = syTsvGet(tsv, row, 1);
		
		/* skip blank rows */
		if (!first && !second)
			continue;
		
		/* table declarator */
		if (first && !strcmp(first, "~"))
			++table;
		/* label name or label-less text */
		else if (first || second)
			table->msgNum += 1;
	}
	
	/* third pass through spreadsheet: populate each table */
	for (table = loc->table - 1, row = 0; row < rowNum; ++row)
	{
		const char *first;
		const char *second;
		
		first = syTsvGet(tsv, row, 0);
		second = syTsvGet(tsv, row, 1);
		
		/* skip blank rows */
		if (!first && !second)
			continue;
		
		/* table declarator */
		if (first && !strcmp(first, "~"))
		{
			++table;
			
			/* allocate space for messages */
			//fprintf(stderr, "table %s has %d messages\n", second, table->msgNum);
			table->msg = calloc(table->msgNum, sizeof(*(table->msg)));
			table->name = second;
			text = table->msg;
			
			/* error checking */
			assert(table->msg);
			assert(table->name);
			/* TODO complain on duplicate table name */
			
			/* count columns in table header */
			for (col = 2; col < colNum; ++col)
				if (!syTsvGet(tsv, row, col))
					break;
			
			/* header is empty */
			if (col == 2)
				continue;
			
			/* allocate and set up table header */
			table->headerNum = col - 2;
			table->header = malloc((col - 2) * sizeof(*(table->header)));
			assert(table->header);
			for (col = 2; col < colNum; ++col)
			{
				const char *name = syTsvGet(tsv, row, col);
				
				if (!name)
					break;
				
				table->header[col - 2] = name;
			}
		}
		/* label name or label-less text */
		else
		{
			text->key.string = first;
			text->key.index = -1;
			text->contents = second;
			text->table = table;
			
			/* populate optional fields */
			if (table->headerNum)
			{
				int udataW = table->headerNum;
				int udataH;
				int i;
				int k;
				
				/* this lookahead determines how many rows udata spans */
				for (udataH = 1; row + udataH < rowNum; ++udataH)
					if (syTsvIsRowBlank(tsv, row + udataH) /* blank row */
						|| syTsvGet(tsv, row + udataH, 0) /* new label or table */
						|| syTsvGet(tsv, row + udataH, 1) /* or label-less text */
					)
						break;
				
				/* construct a udata array pointing to table cells */
				text->udataW = udataW;
				text->udataH = udataH;
				text->udata = malloc(udataH * sizeof(char*));
				assert(text->udata);
				for (i = 0; i < udataH; ++i)
				{
					text->udata[i] = malloc(udataW * sizeof(char*));
					assert(text->udata[i]);
					
					for (k = 0; k < udataW; ++k)
						text->udata[i][k] = syTsvGet(tsv, row + i, k + 2);
				}
				
				/* TODO it currently ignores cells that fall outside
				 * the table's boundaries; change to fatal error later?
				 */
				
				/* simple (unnecessary) optimization to skip udata rows */
				row += udataH - 1;
			}
			
			++text;
		}
	}
	
	/* TODO a future optimization can fit here:
	 * table->label array containing pointers only to messages
	 * containing labels (for faster searching)
	 */
}

void syTextInit(void)
{
	//syTextTest();
	
	g = calloc(1, sizeof(*g));
	
	assert(g);
}

void syTextCleanup(void)
{
	//syTextMachineFree(g);
	g = 0;
}

const char *what(const char *str)
{
	return str ? str : "";
}

const char *syTextGetLabel(const struct syText *t)
{
	if (!t)
		return 0;
	
	return t->key.string;
}

const char *syTextGetContents(const struct syText *t)
{
	if (!t)
		return 0;
	
	return t->contents;
}

const struct syText *syTextNextInArray(const struct syText *t)
{
	if (!t)
		return 0;
	
	return t + 1;
}

void syTextDumpLocale(const char *name, const char *outfn)
{
	FILE *fp = fopen(outfn, "wb");
	struct syTextLocale *locale = FindLocale(name);
	struct syTextTable *table;
	
	for (table = locale->table; table < locale->table + locale->tableNum; ++table)
	{
		struct syText *msg;
		
		if (table != locale->table)
			fprintf(fp, "\n");
		fprintf(fp, "~\t%s", table->name);
		if (table->headerNum)
		{
			for (int i = 0; i < table->headerNum; ++i)
				fprintf(fp, "\t%s", table->header[i]);
		}
		for (msg = table->msg; msg < table->msg + table->msgNum; ++msg)
		{
			fprintf(fp, "\n");
			if (msg->key.string)
				fprintf(fp, "%s", msg->key.string);
			fprintf(fp, "\t%s", msg->contents);
			if (msg->udata)
			{
				for (int i = 0; i < msg->udataH; ++i)
				{
					if (i)
						fprintf(fp, "\n\t");
					for (int k = 0; k < msg->udataW; ++k)
						fprintf(fp, "\t%s", what(msg->udata[i][k]));
				}
			}
		}
	}
	
	fclose(fp);
}

