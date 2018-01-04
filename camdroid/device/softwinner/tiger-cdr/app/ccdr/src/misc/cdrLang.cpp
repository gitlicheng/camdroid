
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>

#include "cdrLang.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "cdrLang.cpp"
#include "debug.h"

using namespace android;

#define maxsize 35
static ssize_t  __getline(char **lineptr, ssize_t *n, FILE *stream)
{
	ssize_t count=0;
	int buf;

	if(*lineptr == NULL) {
		*n=maxsize;
		*lineptr = (char*)malloc(*n);
	}

	if(( buf = fgetc(stream) ) == EOF ) {
		return -1;
	}

	do
	{
		if(buf=='\n') {    
			count += 1;
			break;
		}

		count++;

		*(*lineptr+count-1) = buf;
		*(*lineptr+count) = '\0';

		if(*n <= count)
			*lineptr = (char*)realloc(*lineptr,count*2);
		buf = fgetc(stream);
	} while( buf != EOF);

	return count;
}


cdrLang::cdrLang()
	: lang(LANG_ERR)
	, mLogFont(NULL)
{
	db_info("cdrLang Constructor\n");
	mLangFileTable.clear();

	mLangFileTable.add(LANG_CN,		String8("system/res/lang/zh-CN.bin") );
	mLangFileTable.add(LANG_TW,		String8("system/res/lang/zh-TW.bin") );
	mLangFileTable.add(LANG_EN,		String8("system/res/lang/en.bin") );
	mLangFileTable.add(LANG_JPN,	String8("system/res/lang/jpn.bin") );
	mLangFileTable.add(LANG_KR,		String8("system/res/lang/korean.bin") );
	mLangFileTable.add(LANG_RS,		String8("system/res/lang/russian.bin") );
}


int cdrLang::initLabel(LANGUAGE language)
{
	String8 dataFile;
	ssize_t index;

	db_msg("initLabel");

	index = mLangFileTable.indexOfKey(language);
	if(index >= 0) {
		dataFile = mLangFileTable.valueAt(index);
	} else {
		db_error("invalid language %d\n", language);
		return -1;
	}

	db_msg("language data file is %s\n", dataFile.string());
	if(loadLabelFromLangFile(dataFile) < 0) {
		db_error("load label from %s failed\n", dataFile.string());
		return -1;
	}

	if(loadLabelFromLangFile(dataFile) < 0) {
		db_error("load label from %s failed\n", dataFile.string());
		return -1;
	}
	return 0;
}



int cdrLang::loadLabelFromLangFile(String8 langFile)
{
	FILE *fp;
	char *line = NULL;
	ssize_t len = 0;

	fp = fopen(langFile.string(), "r");
	if (fp == NULL) {
		db_error("open file %s failed: %s\n", langFile.string(), strerror(errno));
		return -1;
	}

	if(label.size() != 0) {
		db_msg("size is %zd\n", label.size());
		label.clear();
		db_msg("size is %zd\n", label.size());
	}
	while ((__getline(&line, &len, fp)) != -1) {
//		db_msg("Retrieved line of length %ld :\n", read);
//		db_msg("%s\n", line);
		label.add(String8(line));
	}

	free(line);
	db_msg("size is %zd\n", label.size());
	fclose(fp);

	return 0;
}

const char* cdrLang::getLabel(unsigned int labelIndex)
{
	if(labelIndex < label.size()) {
		return (const char*)label.itemAt(labelIndex).string();
	}
	else {
		db_msg("invalide label Index: %d, size is %zd\n", labelIndex, label.size());
		return NULL;
	}
}

