#ifndef __JFILEMANAGER_H__
#define __JFILEMANAGER_H__

#include <sys/stat.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
/// Macros
///////////////////////////////////////////////////////////////////////////////

#ifndef BUF_SIZE
#define BUF_SIZE 1024
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
	// 중복 횟수(복사 시 중복된 이름인 경우 카운트)
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
	// 접근 권한(문자열)
	char *mode;
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
void* JFMGetUserData(const JFMPtr fm);
char* JFMGetFileName(const JFMPtr fm, int index);
char* JFMGetFilePath(const JFMPtr fm, int index);
char* JFMGetFileMode(const JFMPtr fm, int index);
JFilePtr JFMGetFile(const JFMPtr fm, int index);
long long JFMGetFileSize(const JFMPtr fm, int index);

// 파일 불러오기(없으면 새로 만들기), 삭제하기
JFMPtr JFMNewFile(JFMPtr fm, const char *name);
JFMPtr JFMDeleteFile(JFMPtr fm, int index);
void JFMDeleteAllFiles(JFMPtr fm);

// 파일 쓰기, 읽기(출력하기)
JFMPtr JFMWriteFile(JFMPtr fm, int index, const char *s, const char *mode);
char** JFMReadFile(JFMPtr fm, int index);

// 파일 검색하기
JFilePtr JFMFindFileByName(const JFMPtr fm, const char *name);

// 파일 이름 변경
JFMPtr JFMRenameFile(JFMPtr fm, int index, const char *newFileName);

// 파일 복사하기, 잘라내기(이동하기)
JFMPtr JFMCopyFile(JFMPtr fm, int index, const char *newFileName);
JFMPtr JFMMoveFile(JFMPtr fm, int index, const char *destPath);

// 파일 크기 변경
JFMPtr JFMTruncateFile(JFMPtr fm, int index, off_t length);

// 파일 접근 권한 바꾸기
JFMPtr JFMChangeMode(JFMPtr fm, int index, const char *mode);

// 파일 상태 및 정보 출력
void JFMPrintFile(const JFMPtr fm, int index);

#endif // #ifndef __JFILEMANAGER_H__

