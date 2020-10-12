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
	EXPECT_NUM_EQUAL(*((int*)(JFMGetUserData(fm))), expected1);

	EXPECT_NULL(JFMSetUserData(NULL, &expected1));
	EXPECT_NULL(JFMSetUserData(fm, NULL));
	EXPECT_NULL(JFMSetUserData(NULL, NULL));

	EXPECT_NULL(JFMGetUserData(NULL));

	JFMDelete(&fm);
})

TEST(FileManager, CreateAndDeleteFile, {
	char *fileName = "fm_test.txt";
	JFMPtr fm = JFMNew();

	// 정상 동작
	EXPECT_NOT_NULL(JFMNewFile(fm, fileName, "r"));
	// 같은 파일 이름으로 재호출 시 NULL 반환
	EXPECT_NULL(JFMNewFile(fm, fileName, "r"));

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

/*

TEST(FileManager, GetFileName, {
	char *fileName = "fm_test.txt";
	JFMPtr fm = JFMNew();

	EXPECT_NOT_NULL(JFMGetFileName(fm));
	EXPECT_STR_EQUAL(JFMGetFileName(fm), fileName);

	JFMDelete(&fm);
})

TEST(FileManager, WriteAndReadFile, {
	char *expected1 = "Hello world!";
	char expected2[12];
	char *fileName = "fm_test.txt";
	JFMPtr fm = JFMNew();
	JFMNewFile(fm, "w+");
	
	EXPECT_NOT_NULL(JFMWriteFile(fm, 0, expected1));
	EXPECT_STR_EQUAL(JFMReadFile(fm, 0)[0], expected1);

	JFMDeleteFile(fm);
	JFMDelete(&fm);
})
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main Function
////////////////////////////////////////////////////////////////////////////////

int main()
{
    CREATE_TESTSUIT();

    REGISTER_TESTS(
		// @ Common Test -----------------------------------------
		Test_FileManager_CreateAndDeleteObject,
		Test_FileManager_SetAndGetUserData,
		Test_FileManager_CreateAndDeleteFile
//		Test_FileManager_GetFileName,
//		Test_FileManager_WriteAndReadFile
    );

    RUN_ALL_TESTS();

    CLEAN_UP_TESTSUIT();

	return 1;
}

