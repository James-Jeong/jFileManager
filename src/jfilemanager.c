#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>
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

typedef enum Bool
{
	False = -1,
	True = 1
} Bool;

typedef enum FileType
{
	Unknown = -1,
	Directory = 1,
	Regular,
	CharDevice,
	BlockDevice,
	Pipe,
	Socket,
	SymbolicLink
} FileType;

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
static char* JFileGetName(const JFilePtr file);
static char* JFileGetPath(const JFilePtr file);
static char* JFileGetMode(const JFilePtr file);
static long long JFileGetSize(const JFilePtr file);
static FILE* JFileOpen(JFilePtr file, const char *mode);
static void JFileClose(const JFilePtr file);
static char* JFileSetName(JFilePtr file, const char *newFileName);
static char* JFileSetPath(JFilePtr file, const char *newFilePath);
static int JFileIncDupleNum(JFilePtr file);

///////////////////////////////////////////////////////////////////////////////
/// Predefinitions of Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

static Bool JFMCheckIndex(JFMPtr fm, int index);
static FileType JFMCheckFileType(JFMPtr fm, int index);
static int JFMFindEmptyFileIndex(JFMPtr fm);

///////////////////////////////////////////////////////////////////////////////
/// Static Util Functions
///////////////////////////////////////////////////////////////////////////////

static Bool _CheckIfPath(const char *s);
static void _ConvertModeToString(char *s, mode_t mode);
static Bool _CheckIfStringIsDigits(const char *s);

///////////////////////////////////////////////////////////////////////////////
/// Static Functions for JFile
///////////////////////////////////////////////////////////////////////////////

/*
 * @fn static JFilePtr JFileNew(const char *name)
 * @brief 파일 정보 관리 구조체 객체를 새로 생성하는 함수
 * @param name 파일 이름(입력, 읽기 전용) 
 * @return 성공 시 생성된 객체의 주소, 실패 시 NULL 반환
 */
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

/*
 * @fn static void JFileDelete(JFilePtrContainer fileContainer)
 * @brief 파일 정보 관리 구조체 객체를 삭제하는 함수
 * @param fileContainer 파일 정보 관리 구조체 객체 주소를 저장하는 포인터(입력, 이중 포인터)
 * @return 반환값 없음
 */
static void JFileDelete(JFilePtrContainer fileContainer)
{
	if((fileContainer == NULL) || (*fileContainer == NULL)) return;
	if((*fileContainer)->name != NULL) free((*fileContainer)->name);
	if((*fileContainer)->path != NULL) free((*fileContainer)->path);
	if((*fileContainer)->mode != NULL) free((*fileContainer)->mode);
	JFileClose(*fileContainer);

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

/*
 * @fn static JFilePtr JFileLoad(JFilePtr file)
 * @brief 파일 정보를 갱신하는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력)
 * @return 성공 시 파일 정보 관리 구조체의 주소, 실패 시 NULL 반환
 */
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

/*
 * @fn static FILE* JFileOpen(JFilePtr file, const char *mode)
 * @brief 지정한 모드로 파일을 여는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력)
 * @param mode 파일 접근 방식(입력, 읽기 전용)
 * @return 성공 시 파일 포인터, 실패 시 NULL 반환
 */
static FILE* JFileOpen(JFilePtr file, const char *mode)
{
	if(file->filePointer != NULL) return NULL;
	file->filePointer = fopen(file->path, mode);
	if(file->filePointer == NULL) return NULL;
	return file->filePointer;
}

/*
 * @fn static void JFileClose(const JFilePtr file)
 * @brief 파일을 닫는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력, 읽기 전용)
 * @return 반환값 없음
 */
static void JFileClose(const JFilePtr file)
{
	if(file->filePointer != NULL)
	{
		if(fclose(file->filePointer) != 0) return;
		file->filePointer = NULL;
	}
}

/*
 * @fn static void JFileRemove(JFilePtr file)
 * @brief 지정한 경로에 있는 파일을 삭제하는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력)
 * @return 반환값 없음
 */
static void JFileRemove(JFilePtr file)
{
	if(file == NULL) return;
	JFileClose(file);
	if((file->path != NULL) && (access(file->path, 0) == 0))
	{
		if(remove(file->path) == -1) return;
	}
}

/*
 * @fn static JFilePtr JFileWrite(JFilePtr file, const char *s, const char *mode)
 * @brief 지정한 모드로 파일을 열어서 전달받은 문자열을 저장하는 함수
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @param s 저장할 문자열(입력, 읽기 전용)
 * @param mode 파일 접근 방식(입력, 읽기 전용)
 * @return 성공 시 파일 정보 관리 구조체의 주소, 실패 시 NULL 반환
 */
static JFilePtr JFileWrite(JFilePtr file, const char *s, const char *mode)
{
	if((file == NULL) || (s == NULL) || (mode == NULL)) return NULL;
	if(JFileOpen(file, mode) == NULL) return NULL;
	if(fputs(s, file->filePointer) < 0) return NULL;
	JFileClose(file);
	if(JFileLoad(file) == NULL) return NULL;
	return file;
}

/*
 * @fn static void JFileDataListClear(JFilePtr file)
 * @brief 파일 관리 구조체에 저장된 파일 내용을 모두 삭제하는 함수
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @return 반환값 없음
 */
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

/*
 * @fn static char** JFileRead(JFilePtr file, int length)
 * @brief 지정한 파일의 전체 내용을 파일 관리 구조체에 저장하는 함수
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @param length 각 라인별 최대 문자 개수(입력)
 * @return 성공 시 저장된 파일 내용, 실패 시 NULL 반환
 */
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

/*
 * @fn static void JFileGetLine(const JFilePtr file)
 * @brief 지정한 파일의 전체 라인수를 구하는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력, 읽기 전용)
 * @return 반환값 업음
 */
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

/*
 * @fn static char* JFileGetName(const JFilePtr file)
 * @brief 지정한 파일의 이름을 반환하는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력, 읽기 전용)
 * @return 성공 시 파일 이름, 실패 시 NULL 반환
 */
static char* JFileGetName(const JFilePtr file)
{
	if(file == NULL) return NULL;
	return file->name;
}

/*
 * @fn static char* JFileGetPath(const JFilePtr file)
 * @brief 지정한 파일의 경로를 반환하는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력, 읽기 전용)
 * @return 성공 시 파일 경로, 실패 시 NULL 반환
 */
static char* JFileGetPath(const JFilePtr file)
{
	if(file == NULL) return NULL;
	return file->path;
}

/*
 * @fn static long long JFileGetSize(const JFilePtr file)
 * @brief 지정한 파일의 크기를 반환하는 함수
 * @param file 파일 정보 관리 구조체의 주소(입력, 읽기 전용)
 * @return 성공 시 파일 크기, 실패 시 -1 반환
 */
static long long JFileGetSize(const JFilePtr file)
{
	if(file == NULL) return -1;
	return file->stat.st_size;
}

/*
 * @fn static char* JFileSetName(JFilePtr file, const char *name)
 * @brief 파일의 이름을 저장하고 동시에 파일의 경로를 구해서 저장하는 함수
 * 사용자가 파일의 경로를 먼저 저장하지 않고 파일의 이름을 저장하면 현재 경로가 설정된다.
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @param name 저장할 파일 이름(입력, 읽기 전용)
 * @return 성공 시 파일 이름, 실패 시 NULL 반환
 */
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

/*
 * @fn static char* JFileSetPath(JFilePtr file, const char *path)
 * @brief 파일의 경로를 저장하고 동시에 파일의 이름을 구해서 저장하는 함수
 * 사용자가 파일의 경로를 먼저 저장하면 파일의 이름을 파싱해서 저장한다.
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @param name 저장할 파일 경로(입력, 읽기 전용)
 * @return 성공 시 파일 경로, 실패 시 NULL 반환
 */
static char* JFileSetPath(JFilePtr file, const char *path)
{
	if(path == NULL) return NULL;
	if(_CheckIfPath(path) == False) return NULL;

	if(file->path != NULL)
	{
		free(file->path);
		file->path = NULL;
	}

	file->path = strdup(path); // malloc
	if(file->path == NULL) return NULL;
	file->path[strlen(file->path)] = '\0';

	// 설정된 경로에서 마지막 '/' 문자 다음 문자열을 파일 이름으로 저장한다.
	char *s = file->path;
	int strIndex = strlen(file->path) - 1;
	for( ; strIndex >= 0; strIndex--)
	{
		if(s[strIndex] == '/') break;
	}

	// 파일 경로가 아니므로 모두 해제하고 NULL 반환
	if(strIndex == 0)
	{
		free(file->path);
		file->path = NULL;
		return NULL;
	}

	strIndex++;
	s += strIndex;

	if(file->name != NULL) free(file->name);
	file->name = strdup(s); // malloc
	if(file->name == NULL) return NULL;

	return file->path;
}

/*
 * @fn static int JFileIncDupleNum(JFilePtr file)
 * @brief 지정한 파일의 중복 횟수를 증가시키는 함수
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @return 항상 증가된 중복 횟수 반환
 */
static int JFileIncDupleNum(JFilePtr file)
{
	return ++(file->dupleNum);
}

/*
 * @fn static char* JFileGetMode(JFilePtr file)
 * @brief 지정한 파일 모드를 문자열로 변환해서 반환하는 함수
 * @param file 파일 정보 관리 구조체의 주소(출력)
 * @return 성공 시 파일 모드 문자열, 실패 시 NULL 반환
 */
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

/*
 * @fn JFMPtr JFMNew()
 * @brief 파일 관리 구조체 객체를 새로 생성하는 함수
 * @return 성공 시 생성된 객체의 주소, 실패 시 NULL 반환
 */
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

/*
 * @fn void JFMDelete(JFMPtrContainer fmContainer)
 * @brief 파일 관리 구조체 객체를 삭제하는 함수
 * @param fmContainer 파일 관리 구조체 객체 주소를 저장하는 포인터(입력, 이중 포인터)
 * @return 반환값 없음
 */
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

/*
 * @fn void* JFMSetUserData(JFMPtr fm, void *userData)
 * @brief 파일 관리 구조체 객체에 사용자 데이터를 저장하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param userData 저장할 데이터의 주소(입력)
 * @return 성공 시 저장된 사용자 데이터의 주소, 실패 시 NULL 반환
 */
void* JFMSetUserData(JFMPtr fm, void *userData)
{
	if((fm == NULL) || (userData == NULL)) return NULL;
	return fm->userData = userData;
}

/*
 * @fn void* JFMGetUserData(const JFMPtr fm)
 * @brief 파일 관리 구조체 객체에 저장된 사용자 데이터를 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(입력, 읽기 전용)
 * @return 성공 시 저장된 사용자 데이터의 주소, 실패 시 NULL 반환
 */
void* JFMGetUserData(const JFMPtr fm)
{
	if(fm == NULL) return NULL;
	return fm->userData;
}

/*
 * @fn JFMPtr JFMNewFile(JFMPtr fm, const char *name)
 * @brief 파일 관리 구조체 객체에 새로운 파일을 추가하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param name 추가할 파일의 이름(입력, 읽기 전용)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
JFMPtr JFMNewFile(JFMPtr fm, const char *name)
{
	if((fm == NULL) || (name == NULL)) return NULL;
	if(_CheckIfPath(name) == True) return NULL;

	int targetIndex = 0;

	//TODO 앞 인덱스의 파일을 삭제하면 해당 인덱스가 비어있게 되므로 여기에 추가해야 한다.
	// 이 인덱스를 찾아서 여기에 파일을 추가하고, realloc 하지 않는다.
	if((targetIndex = JFMFindEmptyFileIndex(fm)) != -1)
	{
		targetIndex = fm->size;
	}

	if(fm->fileContainer[targetIndex] == NULL)
	{
		// 중복 불허, 같은 파일을 동시에 사용할 수 없음
		if(fm->size > 1)
		{
			if(JFMFindFileByName(fm, name) != NULL) return NULL;
		}

		JFilePtrContainer newContainer = fm->fileContainer;
		JFilePtr newFile = JFileNew(name);
		if(newFile == NULL) return NULL;

		if(targetIndex == fm->size)
		{
			newContainer = (JFilePtrContainer)realloc(fm->fileContainer, (fm->size + 1));
			if(newContainer == NULL)
			{
				JFileDelete(&newFile);
				return NULL;
			}
			(fm->size)++;
		}

		newContainer[targetIndex - 1] = newFile;
		newContainer[targetIndex] = NULL;

		fm->fileContainer = newContainer;
		return fm;
	}

	return NULL;
}

/*
 * @fn JFMPtr JFMDeleteFile(JFMPtr fm, int index)
 * @brief 파일 관리 구조체 객체에 저장된 파일을 삭제하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 삭제할 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
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

/*
 * @fn JFMPtr JFMDeleteFile(JFMPtr fm, int index)
 * @brief 파일 관리 구조체 객체에 저장된 파일을 모두 삭제하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @return 반환값 없음
 */
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

/*
 * @fn char* JFMGetFileName(const JFMPtr fm, int index)
 * @brief 지정한 파일의 이름을 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(입력, 읽기 전용)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 이름, 실패 시 NULL 반환
 */
char* JFMGetFileName(const JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return JFileGetName(JFMGetFile(fm, index));
}

/*
 * @fn char* JFMGetFilePath(const JFMPtr fm, int index)
 * @brief 지정한 파일의 경로를 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(입력, 읽기 전용)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 경로, 실패 시 NULL 반환
 */
char* JFMGetFilePath(const JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return JFileGetPath(JFMGetFile(fm, index));
}

/*
 * @fn long long JFMGetFileSize(const JFMPtr fm, int index)
 * @brief 지정한 파일의 크기를 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(입력, 읽기 전용)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 크기, 실패 시 -1 반환
 */
long long JFMGetFileSize(const JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return -1;
	return JFileGetSize(JFMGetFile(fm, index));
}

/*
 * @fn long long JFMGetFileSize(const JFMPtr fm, int index)
 * @brief 지정한 파일을 열어서 전달받은 문자열을 저장하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param s 저장할 문자열(입력, 읽기 전용)
 * @param mode 파일 접근 방식(입력, 읽기 전용)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
JFMPtr JFMWriteFile(JFMPtr fm, int index, const char *s, const char *mode)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (s == NULL)) return NULL;
	if(JFileWrite(JFMGetFile(fm, index), s, mode) == NULL) return NULL;
	return fm;
}

/*
 * @fn char** JFMReadFile(JFMPtr fm, int index)
 * @brief 지정한 파일 내용을 읽어서 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 내용, 실패 시 NULL 반환
 */
char** JFMReadFile(JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return JFileRead(JFMGetFile(fm, index), LINE_LENGTH);
}

/*
 * @fn JFilePtr JFMFindFileByName(const JFMPtr fm, const char *name)
 * @brief 파일 이름을 통해 파일 관리 구조체에서 파일을 검색해서 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(입력, 읽기 전용)
 * @param name 검색할 파일 이름(입력, 읽기 전용)
 * @return 성공 시 파일 정보 구조체의 주소, 실패 시 NULL 반환
 */
JFilePtr JFMFindFileByName(const JFMPtr fm, const char *name)
{
	if((fm == NULL) || (name == NULL)) return NULL;
	if(_CheckIfPath(name) == True) return NULL;

	int nameLength = strlen(name);
	int fileIndex = 0;
	JFilePtr file = NULL;

	for( ; fileIndex < fm->size; fileIndex++)
	{
		file = JFMGetFile(fm, fileIndex);
		if(file == NULL) continue;
		if(strncmp(name, file->name, nameLength) == 0) return file;
	}

	return NULL;
}

/*
 * @fn JFilePtr JFMGetFile(const JFMPtr fm, int index)
 * @brief 지정한 파일 정보 구조체의 주소를 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(입력, 읽기 전용)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 정보 구조체의 주소, 실패 시 NULL 반환
 */
JFilePtr JFMGetFile(const JFMPtr fm, int index)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False)) return NULL;
	return fm->fileContainer[index];
}

/*
 * @fn JFMPtr JFMMoveFile(JFMPtr fm, int index, const char *newFilePath)
 * @brief 파일을 지정한 경로로 이동시키는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @param newFilePath 파일을 이동시킬 경로(입력, 읽기 전용)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
JFMPtr JFMMoveFile(JFMPtr fm, int index, const char *newFilePath)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (newFilePath == NULL)) return NULL;

	// 목적지 경로에 복사 후 삭제
	if(JFMCopyFile(fm, index, newFilePath) == NULL) return NULL;
	JFileRemove(JFMGetFile(fm, index));
	if(JFileSetPath(JFMGetFile(fm, index), newFilePath) == NULL) return NULL;

	return fm;
}

/*
 * @fn JFMPtr JFMCopyFile(JFMPtr fm, int index, const char *newFilePath)
 * @brief 파일을 지정한 경로로 복사하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @param newFilePath 파일을 복사할 경로(입력, 읽기 전용)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
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

/*
 * @fn JFMPtr JFMRenameFile(JFMPtr fm, int index, const char *newFileName)
 * @brief 지정한 파일의 이름을 새로 설정하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @param newFileName 새로 설정할 파일 이름(입력, 읽기 전용)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
JFMPtr JFMRenameFile(JFMPtr fm, int index, const char *newFileName)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (newFileName == NULL)) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	if(rename(file->path, newFileName) == -1) fm = NULL;
	if(JFileSetName(file, newFileName) == NULL) fm = NULL;

	return fm;
}

/*
 * @fn void JFMPrintFile(JFMPtr fm, int index)
 * @brief 지정한 파일의 상태 및 정보를 출력하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @return 반환값 없음
 */
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

/*
 * @fn JFMPtr JFMTruncateFile(JFMPtr fm, int index, off_t length)
 * @brief 지정한 파일의 크기를 새로 설정하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @param length 새로 설정할 파일의 크기(입력)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
JFMPtr JFMTruncateFile(JFMPtr fm, int index, off_t length)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (length < 0)) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	if(truncate(file->path, length) == -1)
	{
//		perror("truncate");
		return NULL;
	}

	if(JFileLoad(file) == NULL) return NULL;

	return fm;
}

/*
 * @fn JFMPtr JFMChangeMode(JFMPtr fm, int index, const char *mode)
 * @brief 지정한 파일의 접근 방식을 새로 설정하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @param mode 새로 설정할 파일 접근 방식(입력, 읽기 전용)
 * @return 성공 시 파일 관리 구조체의 주소, 실패 시 NULL 반환
 */
JFMPtr JFMChangeMode(JFMPtr fm, int index, const char *mode)
{
	if((fm == NULL) || (JFMCheckIndex(fm, index) == False) || (strlen(mode) != 4)) return NULL;
	if(_CheckIfStringIsDigits(mode) == False) return NULL;

	JFilePtr file = JFMGetFile(fm, index);
	if(file == NULL) return NULL;

	long _mode = strtol(mode, 0, 8);
	if((_mode == 0) || (_mode == LONG_MIN) || (_mode == LONG_MAX))
	{
//		perror("strtol");
		return NULL;
	}

	if(chmod(file->path, _mode) == -1) return NULL;
	if(JFileLoad(file) == NULL) return NULL;

	return fm;
}

/*
 * @fn char* JFMGetFileMode(JFMPtr fm, int index)
 * @brief 지정한 파일의 접근 방식을 문자열로 변환해서 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 모드 문자열, 실패 시 NULL 반환
 */
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

/*
 * @fn static Bool JFMCheckIndex(JFMPtr fm, int index)
 * @brief 지정한 인덱스의 경계를 검사하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 True, 실패 시 False 반환(Bool 열거형 참고)
 */
static Bool JFMCheckIndex(JFMPtr fm, int index)
{
	if((index < 0) || (index >= fm->size)) return False;
	return True;
}

/*
 * @fn static FileType JFMCheckFileType(JFMPtr fm, int index)
 * @brief 지정한 파일 유형을 검사하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @param index 파일의 인덱스 번호(입력)
 * @return 성공 시 파일 유형, 실패 시 Unknown 반환(FileType 열거형 참고)
 */
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

/*
 * @fn static int JFMFindEmptyFileIndex(JFMPtr fm)
 * @brief 비어있는 파일 인덱스를 반환하는 함수
 * @param fm 파일 관리 구조체의 주소(출력)
 * @return 성공 시 파일 인덱스, 실패 시 -1 반환
 */
static int JFMFindEmptyFileIndex(JFMPtr fm)
{
	int fileIndex = 0;
	for( ; fileIndex < fm->size; fileIndex++)
	{
		if(fm->fileContainer[fileIndex] == NULL) return fileIndex;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// Static Util Function
///////////////////////////////////////////////////////////////////////////////

/*
 * @fn static Bool _CheckIfPath(const char *s)
 * @brief 지정한 문자열이 경로인지 검사하는 함수
 * @param s 검사할 문자열(입력, 읽기 전용)
 * @return 성공 시 True, 실패 시 False 반환(Bool 열거형 참고)
 */
static Bool _CheckIfPath(const char *s)
{
	if(s == NULL) return False;

	while(*s != '\0')
	{
		if(*s++ == '/') return True;
	}

	return False;
}

/*
 * @fn static void _ConvertModeToString(char *s, mode_t mode)
 * @brief 지정한 파일 접근 방식을 문자열로 변환하는 함수
 * @param s 저장할 문자열(출력)
 * @param mode 변환할 파일 접근 방식(입력)
 * @return 반환값 없음
 */
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

/*
 * @fn static Bool _CheckIfStringIsDigits(const char *s)
 * @brief 지정한 문자열이 숫자 문자들로 구성되어 있는지 검사하는 함수
 * @param s 검사할 문자열(입력, 읽기 전용)
 * @return 성공 시 True, 실패 시 False 반환(Bool 열거형 참고)
 */
static Bool _CheckIfStringIsDigits(const char *s)
{
	int sLength = strlen(s);
	int sIndex = 0;

	for( ; sIndex < sLength; sIndex++)
	{
		if(isdigit(s[sIndex]) == 0) return False;
	}

	return True;
}

