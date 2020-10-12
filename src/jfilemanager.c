#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/jfilemanager.h"

///////////////////////////////////////////////////////////////////////////////
/// Predefinitions of Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

static JFilePtr JFileNew(const char *fileName, const char *mode);
static void JFileDelete(JFilePtrContainer fileContainer);
static void JFileClear(JFilePtr file);
static FILE* JFileLoad(JFilePtr file, const char *mode);
static void JFileRemove(JFilePtr file);
static JFilePtr JFileWrite(JFilePtr file, const char *s);
static char** JFileRead(JFilePtr file, int length);
static void JFileGetLine(const JFilePtr file);
static char* JFileGetName(JFilePtr file);

///////////////////////////////////////////////////////////////////////////////
/// Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

static JFilePtr JFileNew(const char *fileName, const char *mode)
{
	JFilePtr file = (JFilePtr)malloc(sizeof(JFile));
	if(file == NULL) return NULL;

	if(fileName != NULL)
	{
		file->fileName = strdup(fileName);
		if(file->fileName == NULL) return NULL;
		file->fileName[strlen(file->fileName)] = '\0';

		file->filePath = getcwd(NULL, BUF_SIZE);
		if(file->filePath == NULL) return NULL;
		file->filePath[strlen(file->filePath)] = '\0';

		if(strncat(file->filePath, "/", 1) == NULL) return NULL;
		if(strncat(file->filePath, file->fileName, strlen(file->fileName)) == NULL) return NULL;

		int pathLength = strlen(file->filePath);
		char *newFilePath = realloc(file->filePath, pathLength);
		if(newFilePath == NULL)
		{
			free(file);
			free(file->fileName);
			free(file->filePath);
			return NULL;
		}
		file->filePath = newFilePath;

		printf("%s\n", file->filePath);
	}

	file->filePointer = NULL;
	file->dataList = NULL;
	file->line = 1;
	file->totalCharNum = 0;

	if(JFileLoad(file, mode) == NULL)
	{
		JFileDelete(&file);
		return NULL;
	}

	JFileRead(file, LINE_LENGTH);

	return file;
}

static void JFileDelete(JFilePtrContainer fileContainer)
{
	if((fileContainer == NULL) || (*fileContainer == NULL)) return;
	if((*fileContainer)->fileName != NULL) free((*fileContainer)->fileName);
	if((*fileContainer)->filePath != NULL) free((*fileContainer)->filePath);
	if((*fileContainer)->filePointer != NULL) fclose((*fileContainer)->filePointer);

	char **dataList = (*fileContainer)->dataList;
	if(dataList != NULL)
	{
		int dataListIndex = 0;
		for( ; dataListIndex < (*fileContainer)->line; dataListIndex++)
		{
			if(dataList[dataListIndex] != NULL) free(dataList[dataListIndex]);
		}
	}

	free(*fileContainer);
	*fileContainer = NULL;
}

static FILE* JFileLoad(JFilePtr file, const char *mode)
{
	if((file == NULL) || (file->filePath == NULL) || (mode == NULL)) return NULL;

	if(file->filePointer != NULL) fclose(file->filePointer);
	if(access(file->filePath, 0) == -1)
	{
		FILE *tempFilePointer = fopen(file->filePath, "w");
		if(tempFilePointer == NULL) return NULL;
		fclose(tempFilePointer);
	}

	JFileGetLine(file);

	file->filePointer = fopen(file->filePath, mode);
	if(file->filePointer == NULL) return NULL;

	return file->filePointer;
}

static void JFileRemove(JFilePtr file)
{
	if(file == NULL) return;

	if(file->filePointer != NULL)
	{
		if(fclose(file->filePointer) != 0) return;
		file->filePointer = NULL;
	}

	if((file->filePath != NULL) && (access(file->filePath, 0) == 0))
	{
		if(remove(file->filePath) == -1) return;
	}
}

static JFilePtr JFileWrite(JFilePtr file, const char *s)
{
	if((file == NULL) || (file->filePointer == NULL) || (s == NULL)) return NULL;
	if(fputs(s, file->filePointer) < 0) return NULL;
	return file;
}

static void JFileClear(JFilePtr file)
{
	if((file == NULL) || (file->dataList == NULL)) return;

	int fileIndex = 0;
	for( ; fileIndex < file->line; fileIndex++)
	{
		if(file->dataList[fileIndex] != NULL)
		{
			free(file->dataList[fileIndex]);
			file->dataList[fileIndex] = NULL;
		}
	}
}

static char** JFileRead(JFilePtr file, int length)
{
	if((file == NULL) || (file->filePointer == NULL) || (file->line <= 0) || (length <= 0)) return NULL;

	int fileIndex = 0;

	if(file->dataList == NULL)
	{
		file->dataList = (char**)malloc(sizeof(char*) * file->line);
		if(file->dataList == NULL) return NULL;
		for( ; fileIndex < file->line; fileIndex++)
		{
			file->dataList[fileIndex] = NULL;
		}
	}
	else JFileClear(file);

	for(fileIndex = 0; fileIndex < file->line; fileIndex++)
	{
		if(file->dataList[fileIndex] == NULL)
		{
			file->dataList[fileIndex] = (char*)malloc(sizeof(char) * length);
			if(file->dataList[fileIndex] == NULL)
			{
				JFileClear(file);
				return NULL;
			}

			char *s = fgets(file->dataList[fileIndex], length, file->filePointer);
			if(s == NULL)
			{
				free(file->dataList[fileIndex]);
				file->dataList[fileIndex] = NULL;
				break;
			}

			printf("%s\n", file->dataList[fileIndex]);
		}
	}

	return file->dataList;
}

static void JFileGetLine(const JFilePtr file)
{
	char c = '\0';
	FILE *fp = fopen(file->filePath, "r");

	while((c = fgetc(fp)) != EOF)
	{
		if(c == '\n') (file->line)++;
		(file->totalCharNum)++;
	}

	if(file->totalCharNum == 0) file->line = 0;

	fclose(fp);
}

static char* JFileGetName(JFilePtr file)
{
	if(file == NULL) return NULL;
	return file->fileName;
}

///////////////////////////////////////////////////////////////////////////////
/// Functions for JFileManager
///////////////////////////////////////////////////////////////////////////////

JFMPtr JFMNew()
{
	JFMPtr fm = (JFMPtr)malloc(sizeof(JFM));
	if(fm == NULL) return NULL;

	// 맨 처음에 NULL 을 넣어서 데이터 없음을 체크
	fm->fileContainer = (JFilePtrContainer)malloc(sizeof(JFilePtr) * 1);
	if(fm->fileContainer == NULL)
	{
		free(fm);
		return NULL;
	}
	fm->fileContainer[0] = NULL;

	fm->size = 1;
	fm->userData = NULL;

	return fm;
}

void JFMDelete(JFMPtrContainer fmContainer)
{
	if((fmContainer == NULL) || (*fmContainer == NULL)) return;
	JFMDeleteAllFiles(*fmContainer);
	free(*fmContainer);
	*fmContainer = NULL;
}

void* JFMSetUserData(JFMPtr fm, void *userData)
{
	if((fm == NULL) || (userData == NULL)) return NULL;
	return fm->userData = userData;
}

void* JFMGetUserData(JFMPtr fm)
{
	if(fm == NULL) return NULL;
	return fm->userData;
}

JFMPtr JFMNewFile(JFMPtr fm, const char *fileName, const char *mode)
{
	if((fm == NULL) || (fileName == NULL) || (mode == NULL)) return NULL;

	if(fm->fileContainer[fm->size] == NULL)
	{
		// 중복 불허, 같은 파일을 동시에 사용할 수 없음
		if(fm->size > 1)
		{
			if(JFMFindFileByName(fm, fileName) != NULL) return NULL;
		}

		JFilePtr newFile = JFileNew(fileName, mode);
		if(newFile == NULL) return NULL;

		JFilePtrContainer newContainer = (JFilePtrContainer)realloc(fm->fileContainer, (fm->size + 1));
		if(newContainer == NULL)
		{
			JFileDelete(&newFile);
			return NULL;
		}

		newContainer[fm->size - 1] = newFile;
		newContainer[fm->size] = NULL;
		fm->fileContainer = newContainer;
		(fm->size)++;

		return fm;
	}

	return NULL;
}

JFMPtr JFMDeleteFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (fm->fileContainer == NULL) || (fm->size <= 1) || (index < 0) || (index >= fm->size)) return NULL;

	if((fm->fileContainer)[index] != NULL)
	{
		JFileRemove(JFMGetFile(fm, index));
		JFileDelete(&(fm->fileContainer[index]));
	}
	else return NULL;

	(fm->size)--;
	return fm;
}

void JFMDeleteAllFiles(JFMPtr fm)
{
	if(fm == NULL) return;

	if(fm->fileContainer != NULL)
	{
		int fileIndex = 0;
		for( ; fileIndex <= fm->size; fileIndex++)
		{
			JFMDeleteFile(fm, fileIndex);
		}

		free(fm->fileContainer);
	}
}

char* JFMGetFileName(JFMPtr fm, int index)
{
	if((fm == NULL) || (index < 0) || (index >= fm->size)) return NULL;
	return JFileGetName(JFMGetFile(fm, index));
}

JFMPtr JFMWriteFile(JFMPtr fm, int index, const char *s)
{
	if((fm == NULL) || (index < 0) || (index >= fm->size) || (s == NULL)) return NULL;
	if(JFileWrite(JFMGetFile(fm, index), s) == NULL) return NULL;
	return fm;
}

char** JFMReadFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (index < 0) || (index >= fm->size)) return NULL;
	return JFileRead(JFMGetFile(fm, index), LINE_LENGTH);
}

JFilePtr JFMFindFileByName(JFMPtr fm, const char *fileName)
{
	if((fm == NULL) || (fileName == NULL)) return NULL;

	int fileNameLength = strlen(fileName);
	int fileIndex = 0;

	for( ; fileIndex <= fm->size; fileIndex++)
	{
		if(strncmp(fileName, JFMGetFileName(fm, fileIndex), fileNameLength) == 0)
		{
			return JFMGetFile(fm, fileIndex);
		}
	}

	return NULL;
}

JFilePtr JFMGetFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (index < 0) || (index >= fm->size)) return NULL;
	return fm->fileContainer[index];
}

///////////////////////////////////////////////////////////////////////////////
/// Util Functions
///////////////////////////////////////////////////////////////////////////////



