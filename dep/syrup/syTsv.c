/*
 * syTsv.c <SyrupEngine>
 *
 * tsv (tab separated values) spreadsheet parser
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/*
 *
 * private
 *
 */

struct syTsvRow
{
	char **column;
};

struct syTsv
{
	const char *id;
	struct syTsvRow *row;
	int rowNum;
	int columnNum;
	char *tsvDataString;
};

static char *myStrdup(const char *src)
{
	char *dst;
	int srcLen;
	
	if (!src)
		return 0;
	
	srcLen = strlen(src) + 1;
	
	dst = malloc(srcLen);
	assert(dst);
	memcpy(dst, src, srcLen);
	
	return dst;
}

static char *SafeStrdup(struct syTsv *tsv, const char *str)
{
	char *result;
	
	assert(tsv);
	assert(str);
	
	result = myStrdup(str);
	
	assert(result);
	
	return result;
	(void)tsv;
}

static void syTsvFreeMembers(struct syTsv *tsv)
{
	int r;
	
	assert(tsv);
	
	/* cleanup */
	for (r = 0; r < tsv->rowNum; ++r)
	{
		struct syTsvRow *row = tsv->row + r;
		
		if (row->column)
			free(row->column);
	}
	free(tsv->row);
	free(tsv->tsvDataString);
	
	/* reset */
	memset(tsv, 0, sizeof(*tsv));
}

/* count occurrences of c in str */
static int StringCountOccurrences(const char *str, char c)
{
	int n = 0;
	
	while (*str)
		if (*(str++) == c)
			++n;
	
	return n;
}

/* tokenizes a string and returns the number of tokens */
static int Tokenize(char *str, char *delim)
{
	int toks = 0;
	
	assert(str);
	
	while (*str)
	{
		/* if current character is a delimiter */
		if (strchr(delim, *str))
		{
			/* zero all delimiter characters */
			do
			{
				*str = 0;
				++str;
			} while(*str && strchr(delim, *str));
			
			/* and skip the next increment */
			++toks;
			continue;
		}
		
		++str;
	}
	
	return toks;
}

/* https://github.com/sheredom/utf8.h
 * (adapted from utf8valid)
 */
static bool utf8stringIsValid(const char *str)
{
	const char *t = str;
	size_t consumed = 0, remained = 0, n = -1;

	while ((void)(consumed = (size_t)(str - t)), consumed < n && '\0' != *str)
	{
		remained = n - consumed;

		if (0xf0 == (0xf8 & *str))
		{
			/* ensure that there's 4 bytes or more remained */
			if (remained < 4)
				return false;

			/* ensure each of the 3 following bytes in this 4-byte
			 * utf8 codepoint began with 0b10xxxxxx */
			if ((0x80 != (0xc0 & str[1]))
				|| (0x80 != (0xc0 & str[2]))
				|| (0x80 != (0xc0 & str[3]))
			)
				return false;

			/* ensure that our utf8 codepoint ended after 4 bytes */
			if (0x80 == (0xc0 & str[4]))
				return false;

			/* ensure that the top 5 bits of this 4-byte utf8
			 * codepoint were not 0, as then we could have used
			 * one of the smaller encodings */
			if ((0 == (0x07 & str[0])) && (0 == (0x30 & str[1])))
				return false;

			/* 4-byte utf8 code point (began with 0b11110xxx) */
			str += 4;
		}
		else if (0xe0 == (0xf0 & *str))
		{
			/* ensure that there's 3 bytes or more remained */
			if (remained < 3)
				return false;

			/* ensure each of the 2 following bytes in this 3-byte
			 * utf8 codepoint began with 0b10xxxxxx */
			if ((0x80 != (0xc0 & str[1]))
				|| (0x80 != (0xc0 & str[2]))
			)
				return false;

			/* ensure that our utf8 codepoint ended after 3 bytes */
			if (0x80 == (0xc0 & str[3]))
				return false;

			/* ensure that the top 5 bits of this 3-byte utf8
			 * codepoint were not 0, as then we could have used
			 * one of the smaller encodings */
			if ((0 == (0x0f & str[0]))
				&& (0 == (0x20 & str[1]))
			)
				return false;

			/* 3-byte utf8 code point (began with 0b1110xxxx) */
			str += 3;
		}
		else if (0xc0 == (0xe0 & *str))
		{
			/* ensure that there's 2 bytes or more remained */
			if (remained < 2)
				return false;

			/* ensure the 1 following byte in this 2-byte
			 * utf8 codepoint began with 0b10xxxxxx */
			if (0x80 != (0xc0 & str[1]))
				return false;

			/* ensure that our utf8 codepoint ended after 2 bytes */
			if (0x80 == (0xc0 & str[2]))
				return false;

			/* ensure that the top 4 bits of this 2-byte utf8
			 * codepoint were not 0, as then we could have used
			 * one of the smaller encodings */
			if (0 == (0x1e & str[0]))
				return false;

			/* 2-byte utf8 code point (began with 0b110xxxxx) */
			str += 2;
		}
		else if (0x00 == (0x80 & *str))
		{
			/* 1-byte ascii (began with 0b0xxxxxxx) */
			str += 1;
		}
		else
		{
			/* we have an invalid 0b1xxxxxxx utf8 code point entry */
			return false;
		}
	}

	return true;
}

/*
 *
 * public
 *
 */

struct syTsv *syTsvNew(void)
{
	struct syTsv *tsv = calloc(1, sizeof(*tsv));
	
	return tsv;
}

void syTsvFree(struct syTsv *tsv)
{
	assert(tsv);
	
	syTsvFreeMembers(tsv);
	
	free(tsv);
}

char *syTsvGet(struct syTsv *tsv, int row, int column)
{
	assert(tsv);
	assert(row < tsv->rowNum);
	assert(column < tsv->columnNum);
	
	if (row < 0 || column < 0)
		return 0;
	
	/* account for blank row */
	if (!tsv->row[row].column)
		return 0;
	
	return tsv->row[row].column[column];
}

int syTsvColumnIndex(struct syTsv *tsv, int row, const char *name)
{
	int i;
	
	assert(tsv);
	
	for (i = 0; i < tsv->columnNum; ++i)
	{
		const char *colname = tsv->row[row].column[i];
		if (colname && !strcmp(name, colname))
			return i;
	}
	
	return -1;
}

/* get number of rows */
int syTsvGetNumRows(struct syTsv *tsv)
{
	assert(tsv);
	
	return tsv->rowNum;
}

/* get number of columns */
int syTsvGetNumColumns(struct syTsv *tsv)
{
	assert(tsv);
	
	return tsv->columnNum;
}

/* query whether row is blank */
bool syTsvIsRowBlank(struct syTsv *tsv, int row)
{
	assert(tsv);
	
	if (row >= tsv->rowNum)
		return true;
	
	if (!tsv->row[row].column)
		return true;
	
	for (int column = 0; column < tsv->columnNum; ++column)
		if (tsv->row[row].column[column])
			return false;
	
	return true;
}

void syTsvParse(struct syTsv *tsv, const char *id, char *tsvDataString)
{
	char *tsvDataStringEnd;
	int i;
	
	assert(id);
	assert(tsv);
	assert(tsvDataString);
	
	/* only utf8 encoded TSVs are accepted by syTsv */
	if (!utf8stringIsValid(tsvDataString))
	{
		fprintf(stderr, "ERROR: tsv(id='%s') invalid utf8 data string", id);
		exit(EXIT_FAILURE);
	}
	
	/* handle subsequent syTsvParse invocations
	 * (because a syTsv struct can be reused for multiple
	 *  tsvDataStrings if the programmer prefers)
	 */
	if (tsv->tsvDataString)
		syTsvFreeMembers(tsv);
	
	/* set these up for later */
	tsv->id = id;
	tsv->tsvDataString = tsvDataString;
	tsvDataStringEnd = tsvDataString + strlen(tsvDataString);
	
	/* split file string into substrings (one for each row) */
	tsv->rowNum = Tokenize(tsvDataString, "\n\r");
	if (!tsv->rowNum)
		return;
	
	/* just in case there isn't a newline at the end of the file */
	tsv->rowNum += 1;
	
	/* XXX a note regarding the +1's in the upcoming block:
	 * TSV rows don't have column delimiters at the end,
	 * so add 1 to the number of delimiters returned by
	 * StringCountOccurrences() to get the number of columns
	 * (4 delimiters = 5 columns)
	 */
	
	/* allocate space for all the rows */
	tsv->row = calloc(tsv->rowNum, sizeof(*(tsv->row)));
	assert(tsv->row);
	tsv->columnNum = StringCountOccurrences(tsvDataString, '\t') + 1;
	for (i = 0; i < tsv->rowNum; ++i)
	{
		struct syTsvRow *row = tsv->row + i;
		int columnNum = StringCountOccurrences(tsvDataString, '\t') + 1;
		int k;
		
		/* skip completely blank rows */
		if (columnNum == 1)
			continue;
		
		/* this row has a different number of
		 * columns than the first row in the file
		 */
		if (columnNum != tsv->columnNum)
		{
			fprintf(
				stderr
				, "ERROR: TSV(id='%s') row %d different "
				"num columns than first row"
				, tsv->id
				, i + 1
			);
			exit(EXIT_FAILURE);
		}
		
		/* for each column, store a pointer to its data */
		row->column = calloc(columnNum, sizeof(*row->column));
		for (k = 0; k < columnNum; ++k)
		{
			/* end of file */
			if (!*tsvDataString)
			{
				break;
			}
			
			/* leave blank columns empty (cell will point to 0) */
			if (*tsvDataString == '\t')
			{
				++tsvDataString;
				continue;
			}
			
			/* store pointer to column string and advance to next */
			row->column[k] = tsvDataString;
			tsvDataString += strcspn(tsvDataString, "\t");
			
			/* zeroing the delimiters as they're encountered ensures
			 * that the string pointed to by the preceding cell
			 * is terminated properly
			 */
			if (*tsvDataString == '\t')
			{
				*tsvDataString = '\0';
				++tsvDataString;
			}
		}
		
		/* for every row except the last, try advancing to the next */
		if (i < tsv->rowNum - 1)
			while (tsvDataString < tsvDataStringEnd && !*tsvDataString)
				++tsvDataString;
	}
}

void syTsvParseConst(struct syTsv *tsv, const char *id, const char *tsvDataString)
{
	char *dup;
	
	assert(tsvDataString);
	assert(tsv);
	assert(id);
	
	/* only utf8 encoded TSVs are accepted by syTsv */
	if (!utf8stringIsValid(tsvDataString))
	{
		fprintf(stderr, "ERROR: tsv(id='%s') invalid utf8 data string", id);
		exit(EXIT_FAILURE);
	}
	
	dup = SafeStrdup(tsv, tsvDataString);
	
	assert(dup);
	
	syTsvParse(tsv, id, dup);
}

