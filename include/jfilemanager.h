#ifndef __JFILEMANAGER_H__
#define __JFILEMANAGER_H__

#include <string.h>

///////////////////////////////////////////////////////////////////////////////
/// Macro
///////////////////////////////////////////////////////////////////////////////

#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

///////////////////////////////////////////////////////////////////////////////
/// Macro
///////////////////////////////////////////////////////////////////////////////

#ifndef LINE_LENGTH
#define LINE_LENGTH 1024
#endif

///////////////////////////////////////////////////////////////////////////////
/// Definition
///////////////////////////////////////////////////////////////////////////////

typedef struct _jfile_t
{
	int line;
	int totalCharNum;
	FILE *filePointer;
	char *fileName;
	char *filePath;
	char **dataList;
} JFile, *JFilePtr, **JFilePtrContainer;

typedef struct _jfilemanager_t
{
	int size;
	JFilePtrContainer fileContainer;
	void *userData;
} JFM, *JFMPtr, **JFMPtrContainer;

///////////////////////////////////////////////////////////////////////////////
/// Functions for JFileManager
///////////////////////////////////////////////////////////////////////////////

// 파일 매니저 구조체 함수
JFMPtr JFMNew();
void JFMDelete(JFMPtrContainer fmContainer);
void* JFMSetUserData(JFMPtr fm, void *userData);
void* JFMGetUserData(JFMPtr fm);

// 파일 불러오기(없으면 새로 만들기), 삭제하기
JFMPtr JFMNewFile(JFMPtr fm, const char *fileName, const char *mode);
JFMPtr JFMDeleteFile(JFMPtr fm, int index);
void JFMDeleteAllFiles(JFMPtr fm);
char* JFMGetFileName(JFMPtr fm, int index);

// 파일 쓰기, 읽기(출력하기)
JFilePtr JFMGetFile(JFMPtr fm, int index);
JFMPtr JFMWriteFile(JFMPtr fm, int index, const char *s);
char** JFMReadFile(JFMPtr fm, int index);

// 파일 들여쓰기


// 파일 검색하기
JFilePtr JFMFindFileByName(JFMPtr fm, const char *fileName);

// 파일 이동하기


// 파일 권한 바꾸기


#endif // #ifndef __JFILEMANAGER_H__

