/* -*- Mode: C; tab-width: 4; -*- */
/*
* Copyright (C) 2009, HustMoon Studio
*
* 文件名称：myini.c
* 摘	要：读取ini文件+写入ini文件
* 作	者：HustMoon@BYHH
* 修	改：2009.10.8
*/
#include "myini.h"
#include <stdio.h>
#include <string.h>

#include <Windows.h>
#include <WinBase.h>

static LPWSTR CFG_FILE = L".\\conf.ini";	/* 配置文件 */

#define NOT_COMMENT(c)	(c!=';' && c!='#')	/* 不是注释行 */

#ifndef strnicmp
#define strnicmp strncasecmp
#endif

static void getLine(const char *buf, int inStart, int *lineStart, int *lineEnd);
static int findKey(const char *buf, const char *section, const char *key,
	int *sectionStart, int *valueStart, unsigned long *valueSize);
static int getSection(const char *buf, int inStart);

long loadFile(char **buf, const char *fileName)
{
	FILE *fp = NULL;
	long size = 0;
	if ((fp=fopen(fileName, "rb")) == NULL)
		return -1;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	*buf = (char *)malloc(size+1);
	(*buf)[size] = '\0';
	if (fread(*buf, size, 1, fp) < 1) {
		free(*buf);
		size = -1;
	}
	fclose(fp);
	return size;
}

long loadFileW(wchar_t **buf, const char *fileName)
{
	FILE *fp = NULL;
	long size = 0;
	//if ((fp=fopen(fileName, "rb")) == NULL)
	_wfopen_s(&fp,L"c:\\config.conf",L"rb");
	if (fp == NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp)/2;
	rewind(fp);
	*buf = (wchar_t *)malloc(size+1);
	(*buf)[size] = L'\0';
	//跳过unicode标志位 0xfeff
        fread(*buf,2, 1, fp);
	fread(*buf,sizeof(wchar_t), size-1, fp);
	//if (fread(*buf, size-1, 1, fp) < 1) {
	/*	free(*buf);
		size = -1;
	}*/
	fclose(fp);
	return size;
}

static void getLineW(const wchar_t *buf, int inStart, int *lineStart, int *lineEnd)
{
	int start, end;
	for (start=inStart; buf[start]==L' ' || buf[start]==L'\t' || buf[start]==L'\r' || buf[start]==L'\n'; start++);
	for (end=start; buf[end]!=L'\r' && buf[end]!=L'\n' && buf[end]!=L'\0'; end++);
	*lineStart = start;
	*lineEnd = end;
}

static void getLine(const char *buf, int inStart, int *lineStart, int *lineEnd)
{
	int start, end;
	for (start=inStart; buf[start]==' ' || buf[start]=='\t' || buf[start]=='\r' || buf[start]=='\n'; start++);
	for (end=start; buf[end]!='\r' && buf[end]!='\n' && buf[end]!='\0'; end++);
	*lineStart = start;
	*lineEnd = end;
}

static int findKey(const char *buf, const char *section, const char *key,
	int *sectionStart, int *valueStart, unsigned long *valueSize)
{
	int lineStart, lineEnd, i;
	for (*sectionStart=-1, lineEnd=0; buf[lineEnd]!='\0'; )
	{
		getLine(buf, lineEnd, &lineStart, &lineEnd);
		if (buf[lineStart] == '[')
		{
			for (i=++lineStart; i<lineEnd && buf[i]!=']'; i++);
			if (i<lineEnd && strncmp(buf+lineStart, section, i-lineStart)==0)	/* 找到Section？ */
			{
				*sectionStart = lineStart-1;
				if (key == NULL)
					return -1;
			}
			else if (*sectionStart != -1)	/* 找到Section但未找到Key */
				return -1;
		}
		else if (*sectionStart!=-1 && NOT_COMMENT(buf[lineStart]))	/* 找到Section且该行不是注释 */
		{
			for (i=lineStart+1; i<lineEnd && buf[i]!='='; i++);
			if (i<lineEnd && strncmp(buf+lineStart, key, i-lineStart)==0)	/* 找到Key？ */
			{
				*valueStart = i + 1;
				*valueSize = lineEnd - *valueStart;
				return 0;
			}
		}
	}
	return -1;
}

static int findKeyW(const wchar_t *buf, const wchar_t *section, const wchar_t *key,
	int *sectionStart, int *valueStart, unsigned long *valueSize)
{
	int lineStart, lineEnd, i;
	for (*sectionStart=-1, lineEnd=0; buf[lineEnd]!='\0'; )
	{
		getLineW(buf, lineEnd, &lineStart, &lineEnd);
		if (buf[lineStart] == L'[')
		{
			for (i=++lineStart; i<lineEnd && buf[i] != L']'; i++);
			if (i<lineEnd && wcsncmp(buf+lineStart, section, i-lineStart)==0)	/* 找到Section？ */
			{
				*sectionStart = lineStart-1;
				if (key == NULL)
					return -1;
			}
			else if (*sectionStart != -1)	/* 找到Section但未找到Key */
				return -1;
		}
		else if (*sectionStart!=-1 && NOT_COMMENT(buf[lineStart]))	/* 找到Section且该行不是注释 */
		{
			for (i=lineStart+1; i<lineEnd && buf[i] != L'='; i++);
			if (i<lineEnd && wcsncmp(buf+lineStart, key, i-lineStart)==0)	/* 找到Key？ */
			{
				*valueStart = i + 1;
				*valueSize = lineEnd - *valueStart;
				return 0;
			}
		}
	}
	return -1;
}

int getString(const char *buf, const char *section, const char *key,
	const char *defaultValue, char *value, unsigned long size)
{
	int sectionStart, valueStart;
	unsigned long valueSize;

	if (findKey(buf, section, key, &sectionStart, &valueStart, &valueSize)!=0 || valueSize==0)	/* 未找到？ */
	{
		strncpy(value, defaultValue, size);
		return -1;
	}
	if (size-1 < valueSize)		/* 找到但太长？ */
		valueSize = size - 1;
	memset(value, 0, size);
	strncpy(value, buf+valueStart, valueSize);
	return 0;
}

int getStringW(const wchar_t *buf, const wchar_t *section, const wchar_t *key,
	const wchar_t *defaultValue, wchar_t *value, unsigned long size)
{
	int sectionStart, valueStart;
	unsigned long valueSize;

	if (findKeyW(buf, section, key, &sectionStart, &valueStart, &valueSize)!=0 || valueSize==0)	/* 未找到？ */
	{
		wcsncpy(value, defaultValue, size);
		return -1;
	}
	if (size-1 < valueSize)		/* 找到但太长？ */
		valueSize = size - 1;
	memset(value, 0, size);
	wcsncpy(value, buf+valueStart, valueSize);
	return 0;
}

int getInt(const char *buf, const char *section, const char *key, int defaultValue)
{
	char value[21] = {0};
	getString(buf, section, key, "", value, sizeof(value));
	if (value[0] == '\0')	/* 找不到或找到但为空？ */
		return defaultValue;
	return atoi(value);
}

void setStringW(wchar_t **buf, const wchar_t *section, const wchar_t *key, const wchar_t *value)
{
	int sectionStart, valueStart;
	unsigned long valueSize;
	wchar_t *newBuf = NULL;

	if (findKeyW(*buf, section, key, &sectionStart, &valueStart, &valueSize) == 0)	/* 找到key */
	{
		if (value == NULL)	/* 删除key? */
			memmove(*buf+valueStart-wcslen(key)-1, *buf+valueStart+valueSize, 
				wcslen(*buf)+1-valueStart-valueSize);
		else	/* 修改key */
		{
			newBuf = (wchar_t *)malloc(wcslen(*buf)-valueSize+wcslen(value)+1);
			memcpy(newBuf, *buf, valueStart);
			wcscpy(newBuf+valueStart, value);
			wcscpy(newBuf+valueStart+wcslen(value), *buf+valueStart+valueSize);
			free(*buf);
			*buf = newBuf;
		}
	}
	else if (sectionStart != -1)	/* 找到section，找不到key */
	{
		if (key == NULL)	/* 删除section? */
		{
			valueStart = getSectionW(*buf, sectionStart+3);
			if (valueStart <= sectionStart)	/* 后面没有section */
				(*buf)[sectionStart] = L'\0';
			else
				memmove(*buf+sectionStart, *buf+valueStart, wcslen(*buf)+1-valueStart);
		}
		else if (value != NULL)	/* 不是要删除key */
		{
			newBuf = (wchar_t *)malloc(wcslen(*buf)+wcslen(key)+wcslen(value)+4);
			valueSize = sectionStart+wcslen(section)+2;
			memcpy(newBuf, *buf, valueSize);
			wsprintf(newBuf+valueSize, L"\n%s=%s", key, value);
			wcscpy(newBuf+wcslen(newBuf), *buf+valueSize);
			free(*buf);
			*buf = newBuf;
		}
	}
	else	/* 找不到section? */
	{
		if (key!=NULL && value!=NULL)
		{
			newBuf = (wchar_t *)malloc(wcslen(*buf)+wcslen(section)+wcslen(key)+wcslen(value)+8);
			wcscpy(newBuf, *buf);
			wsprintf(newBuf+wcslen(newBuf), L"\n[%s]\n%s=%s", section, key, value);
			free(*buf);
			*buf = newBuf;
		}
	}
}

void setString(char **buf, const char *section, const char *key, const char *value)
{
	int sectionStart, valueStart;
	unsigned long valueSize;
	char *newBuf = NULL;

	if (findKey(*buf, section, key, &sectionStart, &valueStart, &valueSize) == 0)	/* 找到key */
	{
		if (value == NULL)	/* 删除key? */
			memmove(*buf+valueStart-strlen(key)-1, *buf+valueStart+valueSize, 
				strlen(*buf)+1-valueStart-valueSize);
		else	/* 修改key */
		{
			newBuf = (char *)malloc(strlen(*buf)-valueSize+strlen(value)+1);
			memcpy(newBuf, *buf, valueStart);
			strcpy(newBuf+valueStart, value);
			strcpy(newBuf+valueStart+strlen(value), *buf+valueStart+valueSize);
			free(*buf);
			*buf = newBuf;
		}
	}
	else if (sectionStart != -1)	/* 找到section，找不到key */
	{
		if (key == NULL)	/* 删除section? */
		{
			valueStart = getSection(*buf, sectionStart+3);
			if (valueStart <= sectionStart)	/* 后面没有section */
				(*buf)[sectionStart] = '\0';
			else
				memmove(*buf+sectionStart, *buf+valueStart, strlen(*buf)+1-valueStart);
		}
		else if (value != NULL)	/* 不是要删除key */
		{
			newBuf = (char *)malloc(strlen(*buf)+strlen(key)+strlen(value)+4);
			valueSize = sectionStart+strlen(section)+2;
			memcpy(newBuf, *buf, valueSize);
			sprintf(newBuf+valueSize, "\n%s=%s", key, value);
			strcpy(newBuf+strlen(newBuf), *buf+valueSize);
			free(*buf);
			*buf = newBuf;
		}
	}
	else	/* 找不到section? */
	{
		if (key!=NULL && value!=NULL)
		{
			newBuf = (char *)malloc(strlen(*buf)+strlen(section)+strlen(key)+strlen(value)+8);
			strcpy(newBuf, *buf);
			sprintf(newBuf+strlen(newBuf), "\n[%s]\n%s=%s", section, key, value);
			free(*buf);
			*buf = newBuf;
		}
	}
}

static int getSection(const char *buf, int inStart)
{
	int lineStart, lineEnd, i;
	for (lineEnd=inStart; buf[lineEnd]!='\0'; )
	{
		getLine(buf, lineEnd, &lineStart, &lineEnd);
		if (buf[lineStart] == '[')
		{
			for (i=lineStart+1; i<lineEnd && buf[i]!=']'; i++);
			if (i < lineEnd)
				return lineStart;
		}
	}
	return -1;
}

static int getSectionW(const wchar_t *buf, int inStart)
{
	int lineStart, lineEnd, i;
	for (lineEnd=inStart; buf[lineEnd] != L'\0'; )
	{
		getLineW(buf, lineEnd, &lineStart, &lineEnd);
		if (buf[lineStart] == L'[')
		{
			for (i=lineStart+1; i<lineEnd && buf[i] != L']'; i++);
			if (i < lineEnd)
				return lineStart;
		}
	}
	return -1;
}

void setInt(char **buf, const char *section, const char *key, int value)
{
	char svalue[21];
	sprintf(svalue, "%d", value);
	setString(buf, section, key, svalue);
}

int saveFile(const char *buf, const char *fileName)
{
	FILE *fp;
	int result;
	
	if ((fp=fopen(fileName, "wb")) == NULL)
		return -1;
	result = fwrite(buf, strlen(buf), 1, fp)<1 ? -1 : 0;
	fclose(fp);
	return result;
}


void getStringByKeyName(LPWSTR key, LPWSTR buf, DWORD bufSize)
{
	GetPrivateProfileString(L"config",key,NULL,buf,bufSize,CFG_FILE);
}

void writeStringForKeyName(LPWSTR key, LPWSTR value)
{
	WritePrivateProfileString(L"config",key,value,CFG_FILE);
}
