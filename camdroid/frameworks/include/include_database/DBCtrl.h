#ifndef _DBCTRL_H
#define _DBCTRL_H
#include "DBCon.h"
#include "sqlite3.h"


namespace android {
typedef enum
{
	DB_UINT32 = 1,
	DB_INT64,
	DB_UINT16,
	DB_UINT8,
	DB_TEXT,
	DB_BLOB,
	DB_FLOAT,
	DB_DOUBLE,
	DB_DATETIME
}DBC_TYPE;

struct sql_tb
{
    char item[32];
    char type[32];
};

class DBCtrl
{
public:
	DBCtrl();
	~DBCtrl();
	void setSQL(const String8 sql);
	String8 getSQL(void) const;
	String8 getTableName(void) const;
	int insertRecord(void);
	int bufPrepare(void);
	void bufRelease(void);
	int prepare(void);
	int finish(void);
	int deleteRecord(void);
	int executeSQL(void);
	int sqlite3Exec(void);
	unsigned int getRecordCnt(void);
	int getOneLine(void *pValue[]);
	int getFilterFirstLine(void *pValue[]);
	void * getFilterFirstElement(String8 column, int idx);
	void *getElement(String8 columnName, int columnIdx, unsigned int rowNumber);
	int createTable(String8 tbName, struct sql_tb *tb, int cnt);
	void setRecord(String8 columnName, void *pValue);
	void setColumnType(DBC_TYPE *type, int *typeSize);
	void setFilter(const String8 filter);
	int deleteTable(String8 tbName);
private:
	void setTableName(const String8 name);
	void setColumnCnt(const int num);
	void createRequerySQL(void);
	void createAddSQL(void);
	String8 mName;
	String8 mSQL;
	int mColumnCnt;
	int mTmpNum;
	DBC_TYPE *mColumnType;
	int *mColumnTypeSize;
	unsigned int *mValue;
	String8 *mColumnName;
	unsigned char *mFieldKey;
	SQLCon *mSQLCon;
	DBCon *mDBCon;
	sqlite3_stmt *mSqlstmt;
	String8 mFilter;

	unsigned int	result_uint32;
	long long		result_int64;
	unsigned short	result_uint16;
	unsigned char	result_uint8;
	float			result_float;
	double			result_double;
	char (*queryBuf)[100];
};
}
#endif //DBCtrl
