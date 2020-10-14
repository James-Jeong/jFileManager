#include "../include/ttlib.h"
#include "../include/jfilemanager.h"

////////////////////////////////////////////////////////////////////////////////
/// Definitions of Test
////////////////////////////////////////////////////////////////////////////////

DECLARE_TEST();

// ---------- Common Test ----------

////////////////////////////////////////////////////////////////////////////////
/// FileManager Test
////////////////////////////////////////////////////////////////////////////////

TEST(FileManager, CreateAndDeleteObject, {
	JFMPtr fm = JFMNew();
	EXPECT_NOT_NULL(fm);

	JFMDelete(&fm);
	EXPECT_NULL(fm);
})

TEST(FileManager, SetAndGetUserData, {
	int expected1 = 5;
	JFMPtr fm = JFMNew();

	EXPECT_NOT_NULL(JFMSetUserData(fm, &expected1));
	EXPECT_PTR_EQUAL(JFMGetUserData(fm), &expected1);
	EXPECT_NUM_EQUAL(*((int*)(JFMGetUserData(fm))), expected1, int);

	EXPECT_NULL(JFMSetUserData(NULL, &expected1));
	EXPECT_NULL(JFMSetUserData(fm, NULL));
	EXPECT_NULL(JFMSetUserData(NULL, NULL));

	EXPECT_NULL(JFMGetUserData(NULL));

	JFMDelete(&fm);
})

TEST(FileManager, CreateAndDeleteFile, {
	char *fileName = "fm_test.txt";
//	char *filePath = "/home/dev1/src_test/jFileManager/test/fm_test.txt";
	JFMPtr fm = JFMNew();

	// 정상 동작
	EXPECT_NOT_NULL(JFMNewFile(fm, fileName));
	// 같은 파일 이름으로 재호출 시 NULL 반환
	EXPECT_NULL(JFMNewFile(fm, fileName));
	//TODO 파일 이름이 아닌 경로를 매개변수로 사용하면 이름을 파싱하고 경로를 저장
//	EXPECT_NOT_NULL(JFMNewFile(fm, filePath));
	// NULL 입력 시, NULL 반환
	EXPECT_NULL(JFMNewFile(NULL, fileName));
	EXPECT_NULL(JFMNewFile(fm, NULL));
	EXPECT_NULL(JFMNewFile(NULL, NULL));

	// 정상 동작
	EXPECT_NOT_NULL(JFMDeleteFile(fm, 0));
	// 재호출 시 NULL 반환
	EXPECT_NULL(JFMDeleteFile(fm, 0));

	// 인덱스 오류 시 NULL 반환
	EXPECT_NULL(JFMDeleteFile(fm, -1));
	EXPECT_NULL(JFMDeleteFile(fm, 1));
	// 매니저 객체가 NULL 이면 NULL 반환
	EXPECT_NULL(JFMDeleteFile(NULL, 0));

	JFMDelete(&fm);
})


TEST(FileManager, GetFileName, {
	char *fileName = "fm_test.txt";
	JFMPtr fm = JFMNew();

	// 정상 동작
	JFMNewFile(fm, fileName);
	EXPECT_NOT_NULL(JFMGetFileName(fm, 0));
	EXPECT_STR_EQUAL(JFMGetFileName(fm, 0), fileName);

	EXPECT_NULL(JFMGetFileName(NULL, 0));
	EXPECT_NULL(JFMGetFileName(fm, 1));
	EXPECT_NULL(JFMGetFileName(fm, -1));
	EXPECT_NULL(JFMGetFileName(NULL, 1));

	JFMDeleteFile(fm, 0);
	EXPECT_NULL(JFMGetFileName(fm, 0));

	JFMDelete(&fm);
})

TEST(FileManager, GetFilePath, {
	char *fileName = "fm_test.txt";
	char *filePath = "/home/dev1/src_test/jFileManager/test/fm_test.txt";
	JFMPtr fm = JFMNew();

	// 정상 동작
	JFMNewFile(fm, fileName);
	EXPECT_NOT_NULL(JFMGetFilePath(fm, 0));
	EXPECT_STR_EQUAL(JFMGetFilePath(fm, 0), filePath);

	EXPECT_NULL(JFMGetFilePath(NULL, 0));
	EXPECT_NULL(JFMGetFilePath(fm, 1));
	EXPECT_NULL(JFMGetFilePath(fm, -1));
	EXPECT_NULL(JFMGetFilePath(NULL, 1));

	JFMDeleteFile(fm, 0);
	EXPECT_NULL(JFMGetFilePath(fm, 0));

	JFMDelete(&fm);
})

TEST(FileManager, GetFileSize, {
	char *expected1 = "Hello world!\n";
	char *fileName = "fm_test.txt";
	long long fileSize = 39;
	JFMPtr fm = JFMNew();

	// 정상 동작
	JFMNewFile(fm, fileName);

	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NUM_EQUAL(JFMGetFileSize(fm, 0), fileSize, longlong);

	EXPECT_NUM_EQUAL(JFMGetFileSize(NULL, 0), -1, longlong);
	EXPECT_NUM_EQUAL(JFMGetFileSize(fm, 1), -1, longlong);
	EXPECT_NUM_EQUAL(JFMGetFileSize(fm, -1), -1, longlong);
	EXPECT_NUM_EQUAL(JFMGetFileSize(NULL, 1), -1, longlong);

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, GetFileMode, {
	char *expected1 = "Hello world!\n";
	char *fileName = "fm_test.txt";
	char *expectedMode = "rw-rw-r--";
	JFMPtr fm = JFMNew();

	// 정상 동작
	JFMNewFile(fm, fileName);

	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NOT_NULL(JFMGetFileMode(fm, 0));
	EXPECT_STR_EQUAL(JFMGetFileMode(fm, 0), expectedMode);

	EXPECT_NULL(JFMGetFileMode(fm, 1));
	EXPECT_NULL(JFMGetFileMode(NULL, 0));
	EXPECT_NULL(JFMGetFileMode(NULL, 1));
	EXPECT_NULL(JFMGetFileMode(NULL, -1));

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, WriteAndReadFile, {
	char *expected1 = "Hello world!\n";
	char *expected2 = "124 1234y* (*ll2215 asdjfi3\t123\n";
	char *fileName = "fm_test.txt";

	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName);
	
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected2, "a"));

	char **dataList = JFMReadFile(fm, 0);
	EXPECT_STR_EQUAL(dataList[0], expected1);
	EXPECT_STR_EQUAL(dataList[1], expected1);
	EXPECT_STR_EQUAL(dataList[2], expected1);
	EXPECT_STR_EQUAL(dataList[3], expected2);

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, CopyFile, {
	char *expected1 = "Hello world!\n";
	char *fileName = "fm_test.txt";
	char *filePath = "/home/dev1/src_test/jFileManager/fm_test2.txt";

	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName);
	
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NOT_NULL(JFMCopyFile(fm, 0, filePath));
	EXPECT_NOT_NULL(JFMCopyFile(fm, 0, NULL));

	JFMDeleteFile(fm, 0);
	//TODO 특정 경로의 파일도 삭제 가능해야함(복사한 파일 삭제)
//	JFMDeleteFileByPath(fm, 0, filePath);
	
	JFMDelete(&fm);
})

TEST(FileManager, MoveFile, {
	char *expected1 = "Hello world!\n";
	char *fileName = "fm_test.txt";
	char *filePath = "/home/dev1/src_test/jFileManager/fm_test2.txt";

	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName);
	
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NOT_NULL(JFMMoveFile(fm, 0, filePath));

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, TruncateFile, {
	char *expected1 = "Hello world!\n";
	char *fileName = "fm_test.txt";
	off_t fileSize = 1000;

	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName);
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NOT_NULL(JFMTruncateFile(fm, 0, fileSize));
	EXPECT_NUM_EQUAL(JFMGetFileSize(fm, 0), fileSize, longlong);

	EXPECT_NULL(JFMTruncateFile(NULL, 0, fileSize));
	EXPECT_NULL(JFMTruncateFile(fm, -1, fileSize));
	EXPECT_NULL(JFMTruncateFile(fm, 0, -1));
	EXPECT_NULL(JFMTruncateFile(NULL, -1, -1));

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, RenameFile, {
	char *expected1 = "Hello world!\n";
	char *fileName1 = "fm_test1.txt";
	char *fileName2 = "fm_test2.txt";

	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName1);

	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NOT_NULL(JFMRenameFile(fm, 0, fileName2));
	EXPECT_STR_EQUAL(JFMGetFileName(fm, 0), fileName2);

	EXPECT_NULL(JFMRenameFile(NULL, 0, fileName2));
	EXPECT_NULL(JFMRenameFile(fm, -1, fileName2));
	EXPECT_NULL(JFMRenameFile(fm, 0, NULL));
	EXPECT_NULL(JFMRenameFile(NULL, -1, NULL));

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, ChangeModeByNumber, {
	char *expected1 = "Hello world!\n";
	char *fileName = "fm_test.txt";
	char *expectedModeString = "rw-rw-rw-";
	char *expectedMode = "0666";

	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName);

	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "w"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1, "a"));

	EXPECT_NOT_NULL(JFMChangeModeByNumber(fm, 0, expectedMode));
	EXPECT_STR_EQUAL(JFMGetFileMode(fm, 0), expectedModeString);

	EXPECT_NULL(JFMChangeModeByNumber(NULL, 0, expectedMode));
	EXPECT_NULL(JFMChangeModeByNumber(fm, -1, expectedMode));
	EXPECT_NULL(JFMChangeModeByNumber(NULL, -1, expectedMode));

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

TEST(FileManager, FindFileByName, {
	char *fileName = "fm_test.txt";
	JFMPtr fm = JFMNew();
	JFMNewFile(fm, fileName);

	EXPECT_NOT_NULL(JFMFindFileByName(fm, "fm_test.txt"));

	EXPECT_NULL(JFMFindFileByName(fm, "abc.txt"));
	EXPECT_NULL(JFMFindFileByName(fm, NULL));
	EXPECT_NULL(JFMFindFileByName(NULL, "fm_test.txt"));
	EXPECT_NULL(JFMFindFileByName(NULL, NULL));

	JFMDeleteFile(fm, 0);
	JFMDelete(&fm);
})

////////////////////////////////////////////////////////////////////////////////
/// Main Function
////////////////////////////////////////////////////////////////////////////////

int main()
{
    CREATE_TESTSUIT();

    REGISTER_TESTS(
		// @ Common Test -----------------------------------------
		Test_FileManager_CreateAndDeleteObject,
		Test_FileManager_SetAndGetUserData,
		Test_FileManager_CreateAndDeleteFile,
		Test_FileManager_GetFileName,
		Test_FileManager_GetFilePath,
		Test_FileManager_GetFileSize,
		Test_FileManager_GetFileMode,
		Test_FileManager_WriteAndReadFile,
//		Test_FileManager_CopyFile,
		Test_FileManager_MoveFile,
		Test_FileManager_TruncateFile,
		Test_FileManager_RenameFile,
		Test_FileManager_ChangeModeByNumber,
		Test_FileManager_FindFileByName
    );

    RUN_ALL_TESTS();

    CLEAN_UP_TESTSUIT();

	return 1;
}

