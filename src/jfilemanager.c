#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include "../include/jfilemanager.h"

///////////////////////////////////////////////////////////////////////////////
/// Static Definitions
///////////////////////////////////////////////////////////////////////////////

#define MAX_MODE_NUM 9

typedef enum Category
{
	Owner,
	Group,
	Other
} Category;

typedef enum Permission
{
	Read,
	Write,
	Execute
} Permission;

///////////////////////////////////////////////////////////////////////////////
/// Predefinitions of Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

static JFilePtr JFileNew(const char *name);
static void JFileDelete(JFilePtrContainer fileContainer);
static void JFileDataListClear(JFilePtr file);
static JFilePtr JFileLoad(JFilePtr file);
static void JFileRemove(JFilePtr file);
static JFilePtr JFileWrite(JFilePtr file, const char *s, const char *mode);
static char** JFileRead(JFilePtr file, int length);
static void JFileGetLine(const JFilePtr file);
static char* JFileGetName(JFilePtr file);
static char* JFileGetPath(JFilePtr file);
static FILE* JFileOpen(JFilePtr file, const char *mode);
static void JFileClose(JFilePtr file);
static char* JFileSetName(JFilePtr file, const char *newFileName);
static char* JFileSetPath(JFilePtr file, const char *newFilePath);
static int JFileIncDupleNum(JFilePtr file);
static long long JFileGetSize(JFilePtr file);
static char* JFileGetMode(JFilePtr file);

///////////////////////////////////////////////////////////////////////////////
/// Predefinitions of Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

static Bool JFMCheckIndex(JFMPtr fm, int index);
static FileType JFMCheckFileType(JFMPtr fm, int index);

///////////////////////////////////////////////////////////////////////////////
/// Static Util Functions
///////////////////////////////////////////////////////////////////////////////

static Bool _CheckIfPath(const char *s);
static void _ConvertModeToString(char *s, mode_t mode);

///////////////////////////////////////////////////////////////////////////////
/// Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

static JFilePtr JFileNew(const char *name)
{
	JFilePtr file = (JFilePtr)malloc(sizeof(JFile));
	if(file == NULL) return NULL;

	file->name = NULL;
	file->path = NULL;
	file->filePointer = NULL;
	file->dataList = NULL;
	file->mode = NULL;
	file->dupleNum = 0;
	file->line = 0;
	file->totalCharCount = 0;

	if(JFileSetName(file, name) == NULL)
	{
		free(file);
		return NULL;
	}

	if(JFileLoad(file) == NULL)
	{
		JFileDelete(&file);
		return NULL;
	}

	return file;
}

static void JFileDelete(JFilePtrContainer fileContainer)
{
	if((fileContainer == NULL) || (*fileContainer == NULL)) return;
	if((*fileContainer)->name != NULL) free((*fileContainer)->name);
	if((*fileContainer)->path != NULL) free((*fileContainer)->path);
	if((*fileContainer)->mode != NULL) free((*fileContainer)->mode);
	if((*fileContainer)->filePointer != NULL) fclose((*fileContainer)->filePointer);

	if((*fileContainer)->dataList != NULL)
	{
		int dataListIndex = 0;
		for( ; dataListIndex < (*fileContainer)->line; dataListIndex++)
		{
			if(((*fileContainer)->dataList)[dataListIndex] != NULL)
			{
				free(((*fileContainer)->dataList)[dataListIndex]);
			}
		}
		free((*fileContainer)->dataList);
	}

	free(*fileContainer);
	*fileContainer = NULL;
}

static JFilePtr JFileLoad(JFilePtr file)
{
	if((file == NULL) || (file->path == NULL)) return NULL;

	// 파일 열려져 있으면 닫기
	if(file->filePointer != NULL) fclose(file->filePointer);

	// 없으면 새로 생성
	if(access(file->path, 0) == -1)
	{
		FILE *tempFilePointer = fopen(file->path, "w");
		if(tempFilePointer == NULL) return NULL;
		fclose(tempFilePointer);
	}

	// 파일 상태 및 정보 수집
	if(stat(file->path, &(file->stat)) < 0)
	{
		perror("stat");
		return NULL;
	}

	// 파일 라인 수 및 전체 문자 개수 카운트
	JFileGetLine(file);

	// 파일 모드를 문자열로 저장
	if(JFileGetMode(file) == NULL) return NULL;

	return file;
}

static FILE* JFileOpen(JFilePtr file, const char *mode)
{
	if(file->filePointer != NULL) return NULL;
	file->filePointer = fopen(file->path, mode);
	if(file->filePointer == NULL) return NULL;
	return file->filePointer;
}

static void JFileClose(JFilePtr file)
{
	if(file->filePointer != NULL)
	{
		if(fclose(file->filePointer) != 0) return;
		file->filePointer = NULL;
	}
}

static void JFileRemove(JFilePtr file)
{
	if(file == NULL) return;

	JFileClose(file);

	if((file->path != NULL) && (access(file->path, 0) == 0))
	{
		if(remove(file->path) == -1) return;
	}
}

static JFilePtr JFileWrite(JFilePtr file, const char *s, const char *mode)
{
	if((file == NULL) || (s == NULL) || (mode == NULL)) return NULL;
	if(JFileOpen(file, mode) == NULL) return NULL;
	if(fputs(s, file->filePointer) < 0) return NULL;
	JFileClose(file);
	if(JFileLoad(file) == NULL) return NULL;
	return file;
}

static void JFileDataListClear(JFilePtr file)
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
	if((file == NULL) || (file->line <= 0) || (length <= 0)) return NULL;
	if(JFileOpen(file, "r") == NULL) return NULL;

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
	else JFileDataListClear(file);

	for(fileIndex = 0; fileIndex < file->line; fileIndex++)
	{
		if(file->dataList[fileIndex] == NULL)
		{
			file->dataList[fileIndex] = (char*)malloc(sizeof(char) * length);
			if(file->dataList[fileIndex] == NULL)
			{
				JFileDataListClear(file);
				break;
			}

			char *s = fgets(file->dataList[fileIndex], length, file->filePointer);
			if(s == NULL)
			{
				JFileDataListClear(file);
				break;
			}

			int dataLength = strlen(file->dataList[fileIndex]);
			char *newData = (char*)realloc(file->dataList[fileIndex], dataLength + 1);
			if(newData == NULL)
			{
				JFileDataListClear(file);
				break;
			}
			newData[dataLength] = '\0';
			file->dataList[fileIndex] = newData; 
		}
	}

	JFileClose(file);
	return file->dataList;
}

static void JFileGetLine(const JFilePtr file)
{
	char c = '\0';
	FILE *fp = fopen(file->path, "r");
	if(fp == NULL) return;

	file->line = 1;
	while((c = fgetc(fp)) != EOF)
	{
		//개행 문자 다음 문자가 널문자가 아니면 줄 하나 증가
		if(c == '\n')
		{
			if((c = fgetc(fp)) == EOF) break;
			(file->line)++;
		}
		(file->totalCharCount)++;
	}

	if((file->totalCharCount > 0) && (file->line == 0)) file->line = 1;
	fclose(fp);
}

static char* JFileGetName(JFilePtr file)
{
	if(file == NULL) return NULL;
	return file->name;
}

static char* JFileGetPath(JFilePtr file)
{
	if(file == NULL) return NULL;
	return file->path;
}

static long long JFileGetSize(JFilePtr file)
{
	if(file == NULL) return -1;
	return file->stat.st_size;
}

static char* JFileSetName(JFilePtr file, const char *name)
{
	if(name == NULL) return NULL;
	if(_CheckIfPath(name) == True) return NULL;

	if(file->name != NULL)
	{
		if(strncmp(JFileGetName(file), name, strlen(name)) == 0) return NULL;
		else free(file->name);
	}
	if(file->path != NULL) free(file->path);

	file->name = strdup(name); // malloc
	if(file->name == NULL) return NULL;
	file->name[strlen(file->name)] = '\0';

	file->path = getcwd(NULL, BUF_SIZE); // NULL 을 매개변수로 하면, malloc 해서 반환
	if(file->path == NULL)
	{
		free(file->name);
		return NULL;
	}
	file->path[strlen(file->path)] = '\0';

	if(strncat(file->path, "/", 1) == NULL) return NULL;
	if(strncat(file->path, file->name, strlen(file->name)) == NULL) return NULL;

	int pathLength = strlen(file->path);
	char *newFilePath = realloc(file->path, pathLength);
	if(newFilePath == NULL)
	{
		free(file);
		free(file->name);
		free(file->path);
		return NULL;
	}
	file->path = newFilePath;

	return file->name;
}

static char* JFileSetPath(JFilePtr file, const char *path)
{
	if(path == NULL) return NULL;
	if(_CheckIfPath(path) == False) return NULL;

	if(file->path != NULL) free(file->path);

	file->path = strdup(path); // malloc
	if(file->path == NULL) return NULL;
	file->path[strlen(file->path)] = '\0';

	return file->path;
}

static int JFileIncDupleNum(JFilePtr file)
{
	return ++(file->dupleNum);
}

static char* JFileGetMode(JFilePtr file)
{
	if(file->mode == NULL)
	{
		file->mode = (char*)malloc(sizeof(char) * (MAX_MODE_NUM + 1));
		if(file->mode == NULL) return NULL;
	}
	_ConvertModeToString(file->mode, file->stat.st_mode);
	return file->mode;
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

	if((*fmContainer)->fileContainer != NULL)
	{
		int fileIndex = 0;
		int fmSize = (*fmContainer)->size;
		for( ; fileIndex <= fmSize; fileIndex++)
		{
			JFileDelete(&(((*fmContainer)->fileContainer)[fileIndex]));
		}
		free((*fmContainer)->fileContainer);
	}

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

JFMPtr JFMNewFile(JFMPtr fm, const char *name)
{
	if((fm == NULL) || (name == NULL)) return NULL;
	if(_CheckIfPath(name) == True) return NULL;

	if(fm->fileContainer[fm->size] == NULL)
	{
		// 중복 불허, 같은 파일을 동시에 사용할 수 없음
		if(fm->size > 1)
		{
			if(JFMFindFileByName(fm, name) != NULL) return NULL;
		}

		JFilePtr newFile = JFileNew(name);
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
	if((fm == NULL) || (fm->fileContainer == NULL) || (fm->size <= 1) || (JFMCheckIndex(fm, index) == False)) return NULL;

	if((fm->fileContainer)[index] != NULL)
	{
		JFileRemove(JFMGetFile(fm, index));
		JFileDelete(&(fm->fileContainer[index]));
		fm->fileContainer[index] = NULL;
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
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return JFileGetName(JFMGetFile(fm, index));
}

char* JFMGetFilePath(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return JFileGetPath(JFMGetFile(fm, index));
}

long long JFMGetFileSize(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return -1;
	return JFileGetSize(JFMGetFile(fm, index));
}

JFMPtr JFMWriteFile(JFMPtr fm, int index, const char *s, const char *mode)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (s == NULL)) return NULL;
	if(JFileWrite(JFMGetFile(fm, index), s, mode) == NULL) return NULL;
	return fm;
}

char** JFMReadFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return JFileRead(JFMGetFile(fm, index), LINE_LENGTH);
}

JFilePtr JFMFindFileByName(JFMPtr fm, const char *name)
{
	if((fm == NULL) || (name == NULL)) return NULL;
	if(_CheckIfPath(name) == True) return NULL;

	int nameLength = strlen(name);
	int fileIndex = 0;

	for( ; fileIndex <= fm->size; fileIndex++)
	{
		if(strncmp(name, JFMGetFileName(fm, fileIndex), nameLength) == 0)
		{
			return JFMGetFile(fm, fileIndex);
		}
	}

	return NULL;
}

JFilePtr JFMGetFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return fm->fileContainer[index];
}

JFMPtr JFMMoveFile(JFMPtr fm, int index, const char *newFilePath)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (newFilePath == NULL)) return NULL;

	// 목적지 경로에 복사 후 삭제
	if(JFMCopyFile(fm, index, newFilePath) == NULL) return NULL;
	JFileRemove(JFMGetFile(fm, index));
	if(JFileSetPath(JFMGetFile(fm, index), newFilePath) == NULL) return NULL;

	return fm;
}

JFMPtr JFMCopyFile(JFMPtr fm, int index, const char *newFilePath)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	if(_CheckIfPath(newFilePath) == False) return NULL;

	FILE *newFilePointer = NULL;
	if(newFilePath == NULL)
	{
		char *oldFilePath = strdup(JFMGetFile(fm, index)->path);
		char dupleBuf[BUF_SIZE];

		sprintf(dupleBuf, "_%d", JFMGetFile(fm, index)->dupleNum + 1);
		strcat(oldFilePath, dupleBuf);

		newFilePointer = fopen(oldFilePath, "w");

		free(oldFilePath);
		JFileIncDupleNum(JFMGetFile(fm, index));
	}
	else newFilePointer = fopen(newFilePath, "w");
	if(newFilePointer == NULL) return NULL;

	if(JFileOpen(JFMGetFile(fm, index), "r") == NULL) return NULL;

	char c = '\0';
	FILE *oldFilePointer = JFMGetFile(fm, index)->filePointer;

	while(1)
	{
		c = fgetc(oldFilePointer);
		if(!feof(oldFilePointer)) fputc(c, newFilePointer);
		else break;
	}

	fclose(newFilePointer);
	JFileClose(JFMGetFile(fm, index));
	return fm;
}

JFMPtr JFMRenameFile(JFMPtr fm, int index, const char *newFileName)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (newFileName == NULL)) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	char *oldFilePath = strdup(file->path);
	if(JFileSetName(file, newFileName) == NULL) fm = NULL;
	if(rename(oldFilePath, file->path) == -1) fm = NULL;
	free(oldFilePath);

	return fm;
}

void JFMPrintFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return;

	printf("\n----------------------------------\n");
	printf("File Name : %s\n", file->name);
	printf("File Path : %s\n", file->path);
	printf("File Line Count : %d\n", file->line);
	printf("File Char Count : %d\n", file->totalCharCount);
	printf("I-Node Number : %ld\n", (long)(file->stat.st_ino));
	printf("Mode : %lo (octal)\n", (unsigned long)(file->stat.st_mode));
	printf("Link Count : %ld\n", (long)(file->stat.st_nlink));
	printf("Ownership : UID=%ld / GID=%ld\n", (long)(file->stat.st_uid), (long)(file->stat.st_gid));
	printf("Preferred I/O Block Size : %ld bytes\n", (long)(file->stat.st_blksize));
	printf("File Size : %lld bytes\n", (long long)(file->stat.st_size));
	printf("Blocks allocated %lld\n", (long long)(file->stat.st_blocks));
	printf("Last Status Change : %s", ctime(&(file->stat.st_ctime)));
	printf("Last File Access : %s", ctime(&(file->stat.st_atime)));
	printf("Last File Modification : %s", ctime(&(file->stat.st_mtime)));
	printf("----------------------------------\n");
}

JFMPtr JFMTruncateFile(JFMPtr fm, int index, off_t length)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (length < 0)) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	if(truncate(file->path, length) == -1)
	{
		perror("truncate");
		return NULL;
	}

	if(JFileLoad(file) == NULL) return NULL;

	return fm;
}

JFMPtr JFMChangeModeByNumber(JFMPtr fm, int index, const char *mode)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (strlen(mode) != 4)) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	int _mode = strtol(mode, 0, 8);

	if(chmod(file->path, _mode) == -1) return NULL;
	if(JFileLoad(file) == NULL) return NULL;

	return fm;
}

char* JFMGetFileMode(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	return JFileGetMode(file);
}

///////////////////////////////////////////////////////////////////////////////
/// Static Functions for JFileManager
///////////////////////////////////////////////////////////////////////////////

static Bool JFMCheckIndex(JFMPtr fm, int index)
{
	if((index < 0) || (index >= fm->size)) return False;
	return True;
}

static FileType JFMCheckFileType(JFMPtr fm, int index)
{
	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return;

	switch(file->stat.st_mode & S_IFMT)
	{
		case S_IFREG: return Regular;
		case S_IFDIR: return Directory;
		case S_IFCHR: return CharDevice;
		case S_IFBLK: return BlockDevice;
		case S_IFIFO: return Pipe;
		case S_IFLNK: return SymbolicLink;
		case S_IFSOCK: return Socket;
	}
	return Unknown;
}

///////////////////////////////////////////////////////////////////////////////
/// Static Util Function
///////////////////////////////////////////////////////////////////////////////

static Bool _CheckIfPath(const char *s)
{
	if(s == NULL) return False;

	while(*s != '\0')
	{
		if(*s++ == '/') return True;
	}

	return False;
}

static void _ConvertModeToString(char *s, mode_t mode)
{
	const char basePermissions[MAX_MODE_NUM] = "rwxrwxrwx";
	int modeIndex = 0;

	for( ; modeIndex < MAX_MODE_NUM; modeIndex++)
	{
		s[modeIndex] = (mode & (1 << (MAX_MODE_NUM - modeIndex - 1))) ? basePermissions[modeIndex] : '-';
	}

	s[MAX_MODE_NUM] = '\0';
}

