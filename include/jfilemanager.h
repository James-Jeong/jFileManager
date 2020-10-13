#ifndef __JFILEMANAGER_H__
#define __JFILEMANAGER_H__

#include <sys/stat.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
/// Enums
///////////////////////////////////////////////////////////////////////////////

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
/// Macros
///////////////////////////////////////////////////////////////////////////////

#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

#ifndef LINE_LENGTH
#define LINE_LENGTH 1024
#endif

///////////////////////////////////////////////////////////////////////////////
/// Definition
///////////////////////////////////////////////////////////////////////////////

typedef struct stat FileStatus, *FileStatusPtr, **FileStatusPtrContainer;

typedef struct _jfile_t
{
	// 복사 시 중복된 이름인 경우 카운트
	int dupleNum;
	// 전체 라인 수
	int line;
	// 전체 문자 개수
	int totalCharCount;
	// 파일 포인터
	FILE *filePointer;
	// 이름
	char *name;
	// 경로
	char *path;
	// 전체 문자열 배열(개행으로 구분)
	char **dataList;
	// 파일 상태 및 정보
	FileStatus stat;
} JFile, *JFilePtr, **JFilePtrContainer;

typedef struct _jfilemanager_t
{
	// 파일 개수
	int size;
	// 전체 파일 관리 배열
	JFilePtrContainer fileContainer;
	// 사용자 데이터
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
char* JFMGetFileName(JFMPtr fm, int index);
char* JFMGetFilePath(JFMPtr fm, int index);
long long JFMGetFileSize(JFMPtr fm, int index);

// 파일 불러오기(없으면 새로 만들기), 삭제하기
JFMPtr JFMNewFile(JFMPtr fm, const char *name);
JFMPtr JFMDeleteFile(JFMPtr fm, int index);
void JFMDeleteAllFiles(JFMPtr fm);

// 파일 쓰기, 읽기(출력하기)
JFilePtr JFMGetFile(JFMPtr fm, int index);
JFMPtr JFMWriteFile(JFMPtr fm, int index, const char *s, const char *mode);
char** JFMReadFile(JFMPtr fm, int index);

// 파일 검색하기
JFilePtr JFMFindFileByName(JFMPtr fm, const char *name);

// 파일 이름 변경
JFMPtr JFMRenameFile(JFMPtr fm, int index, const char *newFileName);

// 파일 복사하기, 잘라내기(이동하기)
JFMPtr JFMCopyFile(JFMPtr fm, int index, const char *newFileName);
JFMPtr JFMMoveFile(JFMPtr fm, int index, const char *destPath);

// 파일 크기 변경
JFMPtr JFMTruncateFile(JFMPtr fm, int index, off_t length);

// 파일 권한 바꾸기


// 파일 상태 및 정보 출력void JFMPrintFile(JFMPtr fm, int index);
void JFMPrintFile(JFMPtr fm, int index);

#endif // #ifndef __JFILEMANAGER_H__

