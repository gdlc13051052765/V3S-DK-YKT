/*****************************************************************************************
 * 文件说明：
 * SQLITE3 数据库管理写盘器配置数据库，菜单数据库的创建，查询，插入，修改等
 * 
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "sqlite3.h"
#include "sqliteTask.h"
#include "debug_print.h"
#include "msgTask.h"
#include "mydefine.h"

static char *zErrMsg =NULL;
static char **azResult=NULL; //二维数组存放结果
static int nrow=0;
static int ncolumn = 0;
sqlite3 *recod_db=NULL;//未采记录数据库句柄
sqlite3 *blknumber_db=NULL;//黑名单数据库句柄
sqlite3 *moneyplan_db=NULL;//价格方案数据库句柄

static bool recordMux =false;//数据库操作加锁

//上传的交易记录
struct sRecordStruct recordSt;
uint32_t blknumberId = 0;

static char version[] = "YKT-VER-0.0.1";//版本号
static int  maincode ;
static char ipaddr[] = "192.168.1.100"; 
static int  port = 9000;
static char MatchCode[] = "12345678";
static char CardKeyCode[] = "12345678";
static char CalCardKey[] = "12345678";
static int  CardMinBalance = 100;
static int  DayLimetMoney = 100000;
static char CommEncryptKey[] = "12345678"; 
static char CardBatchEnable[] = "123456788";
static char PurseEnable[] = "12345678";
static int  consumeMode = 1;
static int  cardSector = 3;//公共扇区


char *code;


// Note: Typical values of SCNd64 include "lld" and "ld".
//字符串转int64
static int64_t S64(const char *s) 
{
  int64_t i;
  char c ;
  int scanned = sscanf(s, "%" SCNd64 "%c", &i, &c);
  if (scanned == 1) return i;
  if (scanned > 1) {
    // TBD about extra data found
    return i;
  }
  // TBD failed to scan;  
  return 0;  
}

/*==================================================================================
* 函 数 名： sqlite_create_config_db
* 参    数： 
* 功能描述:  创建配置数据库
* 返 回 值： None
* 备    注： 创建成功返回0
* 作    者： lc
* 创建时间： 2021-09-27
==================================================================================*/
int sqlite_create_config_db(void)
{
  int err;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建进程保护数据库 */
  err = sqlite3_open("/home/meican/ykt_config.db",&config_db);
  if( err ) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close(config_db);
    return err;
  }
  else 
    printf_debug("You have opened a sqlite3 database named config_db successfully!\n");
  //
  char *sql = "create table config_db (version char, maincode INTEGER, ipaddr char, port INTEGER, MatchCode char, CardKeyCode char, CalCardKey char, CardMinBalance INTEGER, DayLimetMoney INTEGER, CommEncryptKey char, CardBatchEnable char, PurseEnable char, consumeMode INTEGER, cardSector INTEGER);";
  sqlite3_exec(config_db,sql,NULL,NULL,&zErrMsg);
  sqlite3_close(config_db);

  //插入数据
  err = sqlite3_open("/home/meican/ykt_config.db",&config_db);
  if( err ) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close(config_db);
    return err;
  }
  sprintf(tempdata, "insert into config_db values('%s',%d,'%s',%d,'%s','%s','%s',%d,%d,'%s','%s','%s',%d,%d);", version, maincode,ipaddr,port,MatchCode,CardKeyCode,CalCardKey,CardMinBalance,DayLimetMoney,CommEncryptKey,CardBatchEnable,PurseEnable,consumeMode,cardSector);

  printf_debug("config_db insertdata = %s\n", tempdata);
  err = sqlite3_exec(config_db,tempdata,NULL,NULL,&zErrMsg);
  sqlite3_close(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_read_devMsg_from_config_db
* 参    数： 
* 功能描述:  从配置数据库读取设备信息
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 
==================================================================================*/
static  char pIpAddr[100];
static  char pMatchCode[100];
static  char pCardKeyCode[100];
static  char pCalCardKey[100];
static  char pCommEncryptKey[100];
static  char pCardBatchEnable[100];
static  char pPurseEnable[100];

C_DevMsgSt sqlite_read_devMsg_from_config_db()
{
  int err =-1,dev_status = -1,i,hardNum;
  int nrow,ncolumn;
  char * errmsg = NULL;
  char **azResult; //是 char ** 类型，两个*号
  char gloalVersion[50];
  sqlite3 *config_db =NULL;
  C_DevMsgSt pdev ;

  /* 创建基础信息数据库 */
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READONLY, NULL);
  if( err ) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return pdev;
  }
  //配置数据库读取数据
  char *sql = "select * from config_db";
  err = sqlite3_get_table(config_db, sql, &azResult, &nrow, &ncolumn, &zErrMsg);
  if( err ) {
    printf_debug("Can't select from config: %s\n", sqlite3_errmsg(config_db));
    sqlite3_free_table(azResult);
    sqlite3_free(zErrMsg);
    sqlite3_close_v2(config_db);
    return pdev;
  }
  for(i=0;i<(nrow+1)*ncolumn;i++)
  {
    printf_debug("azResult[%d]=%s\n",i,azResult[i]);
  }
  hardNum = 13;
  memcpy(gloalVersion, azResult[hardNum], strlen(azResult[hardNum]));
  pdev.version = gloalVersion;
  pdev.maincode = atoi(azResult[hardNum+1]);
  memcpy(pIpAddr, azResult[hardNum+2], strlen(azResult[hardNum+2]));
  pdev.ipaddr = pIpAddr;
  pdev.port = atoi(azResult[hardNum+3]);
  memcpy(pMatchCode, azResult[hardNum+4], strlen(azResult[hardNum+4]));
  pdev.matchCode = pMatchCode;
  memcpy(pCardKeyCode, azResult[hardNum+5], strlen(azResult[hardNum+5]));
  pdev.cardKeyCode = pCardKeyCode;
  memcpy(pCalCardKey, azResult[hardNum+6], strlen(azResult[hardNum+6]));
  pdev.calCardKey = pCalCardKey;
  pdev.cardMinBalance = atoi(azResult[hardNum+7]);
  pdev.dayLimetMoney = atoi(azResult[hardNum+8]);
  memcpy(pCommEncryptKey, azResult[hardNum+9], strlen(azResult[hardNum+9]));
  pdev.commEncryptKey = pCommEncryptKey;
  memcpy(pCardBatchEnable, azResult[hardNum+10], strlen(azResult[hardNum+10]));
  pdev.cardBatchEnable = pCardBatchEnable;
  memcpy(pPurseEnable, azResult[hardNum+11], strlen(azResult[hardNum+11]));
  pdev.purseEnable = pPurseEnable;
  pdev.consumeMode = atoi(azResult[hardNum+12]);

  printf_debug("version = %s \n", pdev.version);
  printf_debug("maincode = %d \n", pdev.maincode);
  printf_debug("ipaddr = %s \n", pdev.ipaddr);
  printf_debug("port = %d \n", pdev.port);
  printf_debug("MatchCode = %s \n", pdev.matchCode);
  printf_debug("CardKeyCode = %s \n", pdev.cardKeyCode);
  printf_debug("CalCardKey = %s \n", pdev.calCardKey);
  printf_debug("CardMinBalance = %d \n", pdev.cardMinBalance);
  printf_debug("DayLimetMoney = %d \n", pdev.dayLimetMoney);
  printf_debug("CommEncryptKey = %s \n", pdev.commEncryptKey);
  printf_debug("CardBatchEnable = %s \n", pdev.cardBatchEnable);
  printf_debug("PurseEnable = %s \n", pdev.purseEnable);
  printf_debug("consumeMode = %d \n", pdev.consumeMode);

  sqlite3_free_table(azResult);
  sqlite3_free(zErrMsg);
  sqlite3_close_v2(config_db);

  return pdev;
}

/*==================================================================================
* 函 数 名： sqlite_update_matchCode_config_db
* 参    数： 
* 功能描述:  修改配置数据库匹配字
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_matchCode_config_db(char *snbuf)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set MatchCode='%s'", snbuf);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_cardKeyCode_config_db
* 参    数： 
* 功能描述:  修改配置数据库读卡密钥
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardKeyCode_config_db(char *snbuf)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set CardKeyCode='%s'", snbuf);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_cardSector_config_db
* 参    数： 
* 功能描述:  修改配置数据库公共扇区
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardSector_config_db(int cardSector)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set cardSector=%d", cardSector);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_CalCardKey_config_db
* 参    数： 
* 功能描述:  修改配置数据库匹配字
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_CalCardKey_config_db(char *CalCardKey)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set CalCardKey='%s'", CalCardKey);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_cardBatchEnable_config_db
* 参    数： 
* 功能描述:  修改配置数据库卡钱包
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //  
==================================================================================*/
int sqlite_update_cardBatchEnable_config_db(char *cardBatchEnable)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set CardBatchEnable='%s'", cardBatchEnable);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_commEncryptKey_config_db
* 参    数： 
* 功能描述:  修改配置数据库传输密钥
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //  
==================================================================================*/
int sqlite_update_commEncryptKey_config_db(char *commEncryptKey)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set CommEncryptKey='%s'", commEncryptKey);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_cardMinBalance_config_db
* 参    数： 
* 功能描述:  修改配置数据库卡底金
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardMinBalance_config_db(int cardMinBalance)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set CardMinBalance=%d", cardMinBalance);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

/*==================================================================================
* 函 数 名： sqlite_update_cardMinBalance_config_db
* 参    数： 
* 功能描述:  修改配置数据库日限额
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //  
==================================================================================*/
int sqlite_update_dayLimetMoney_config_db(int dayLimetMoney)
{
  int err,codeNum;
  char tempdata[500];
  sqlite3 *config_db =NULL;

  /* 创建基础信息数据库 */
  //err = sqlite3_open("/home/meican/base_config.db",&config_db);
  err = sqlite3_open_v2("/home/meican/ykt_config.db", &config_db, SQLITE_OPEN_READWRITE, NULL);
  if(err) {
    printf_debug("Can't open database: %s\n", sqlite3_errmsg(config_db));
    sqlite3_close_v2(config_db);
    return err;
  }
  codeNum = atoi(code);
  sprintf(tempdata, "update config_db set DayLimetMoney=%d", dayLimetMoney);//
  printf_debug("tempdata = %s \n", tempdata);
  err = sqlite3_exec(config_db, tempdata, NULL, NULL, &zErrMsg);
  sqlite3_close_v2(config_db);
  return err;
}

//创建价格方案数据库
int sqlite3_moneyplan_open_db(void)
{
     int len;
	 
	/* 打开或创建未采记录数据库 */
	len = sqlite3_open("moneyplan.db",&moneyplan_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(moneyplan_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(moneyplan_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named moneyplan_db successfully!\n");
	
	/* 创建未采记录表 */
	//recordIdb 一直累加不清零
	//消费时间年/月/日
	char *sql = "create table moneyplan (id int,datas char);" ;
	sqlite3_exec(moneyplan_db,sql,NULL,NULL,&zErrMsg);
	//sqlite3_close(recod_db);
	return len;
}


//插入价格方案到数据库
int sqlite3_moneyplan_insert_db(struct sMoneyplanStruct stru)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("moneyplan.db",&moneyplan_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(moneyplan_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(moneyplan_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"insert into moneyplan values(%d,'%s');",stru.serid,stru.recoedDatas);
	printf_debug("blknumber tempdata= %s\n", tempdata);
	sqlite3_exec(moneyplan_db,tempdata,NULL,NULL,&zErrMsg);
	sqlite3_close(moneyplan_db);
}

//查找符合身份的价格方案
struct sMoneyplanStruct sqlite3_moneyplan_query_db(uint32_t serid )
{
	char tempdata[100],bufer[40];
	int len,numberbak =-1;
	const char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的
	memset(bufer,10,0);
	memset(tempdata,100,0);
	struct sMoneyplanStruct stru;
	int ret =-1;

	/* 打开数据库 */
	len = sqlite3_open("moneyplan.db",&moneyplan_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(moneyplan_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(moneyplan_db);
	   return stru;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"select *from moneyplan where id = %d;",serid);
	printf_debug("moneyplan tempdata= %s\n", tempdata);

	if (sqlite3_prepare(moneyplan_db, tempdata, -1, &stmt, NULL) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			buf = sqlite3_column_text(stmt, 1);
			memcpy(stru.recoedDatas,buf,30);
			printf_debug("data =%s\n",buf);
			return stru;
		}
	}
	return stru;//没有找到符合身份的价格方案
}
//查找价格方案存储记录指针
uint32_t  sqlite3_query_moneyplan_db(void)
{
	int index =0,indexbak=0,len;
	sqlite3_stmt *stmt = NULL; // 用来取数据的
	bool start =false;

	/* 打开数据库 */
	len = sqlite3_open("moneyplan.db",&moneyplan_db);
	if( len )
	{
		/*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(moneyplan_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
		sqlite3_close(moneyplan_db);
		return 0;
	}
	else 
		printf("You have opened a sqlite3 database named record successfully!\n");
 
	// 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// -1代表系统会自动计算SQL语句的长度
	char *sql="select *from moneyplan where id ";
	if (sqlite3_prepare_v2(moneyplan_db, sql, -1, &stmt, NULL) == SQLITE_OK) 
	{   
		while(sqlite3_step(stmt) == SQLITE_ROW ) 
		{	
			// 取出第0列字段的值ID号
			index= sqlite3_column_int(stmt, 0);	
			index++;
		}

		sqlite3_finalize(stmt);
		sqlite3_close(moneyplan_db);
		return index;	 
	}
  return index;
}

//清空价格方案数据库
int sqlite3_moneyplan_clr_db(void)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("moneyplan.db",&moneyplan_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(moneyplan_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(moneyplan_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"delete from moneyplan_db;");
	printf_debug("blknumber tempdata= %s\n", tempdata);
	sqlite3_exec(moneyplan_db,tempdata,NULL,NULL,&zErrMsg);
	sqlite3_close(moneyplan_db);
}


//如果黑名单数据库存在打开数据库，不存在就创建数据库
int sqlite3_blaknumber_open_db(void)
{
  int len;
	 
	/* 打开或创建未采记录数据库 */
	len = sqlite3_open("blknum.db",&blknumber_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(blknumber_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(blknumber_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named recod_db successfully!\n");
	
	/* 创建未采记录表 */
	//recordIdb 一直累加不清零
	//消费时间年/月/日
	char *sql = "create table blknumber (id int,number int);" ;

	sqlite3_exec(blknumber_db,sql,NULL,NULL,&zErrMsg);
	//sqlite3_close(recod_db);
	return len;
}

//查询是否在黑名单数据库
int sqlite3_blaknumber_query_db(uint32_t number)
{
	char tempdata[100],bufer[10];
	int len,numberbak =-1;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的
	memset(bufer,10,0);
	memset(tempdata,100,0);

	int ret =-1;

	/* 打开数据库 */
	len = sqlite3_open("blknum.db",&blknumber_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(blknumber_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(blknumber_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"select *from blknumber where number = %d;",number);
	printf_debug("blknumber tempdata= %s\n", tempdata);

	ret = -1;
	if (sqlite3_prepare(blknumber_db, tempdata, -1, &stmt, NULL) == SQLITE_OK) {
		//sqlite3_bind_int(stmt, 0, CardNum);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			numberbak = sqlite3_column_int(stmt, 1);
			if (numberbak == number) {
				break;
			}
		}
	}
	if(numberbak == number)
	{
		printf_debug("黑名单卡\n");
		sqlite3_finalize(stmt);
		sqlite3_close(blknumber_db);
		return 1;
	}
	else
	{
		printf_debug("正常卡卡\n");
		sqlite3_finalize(stmt);
		sqlite3_close(blknumber_db);
		return 0;
	}
}

//插入黑名单数据库
int sqlite3_blaknumber_insert_db(uint32_t number)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	if(sqlite3_blaknumber_query_db(number))//数据库中已经存在
		return;

	/* 打开数据库 */
	len = sqlite3_open("blknum.db",&blknumber_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(blknumber_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(blknumber_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"insert into blknumber values(%d,%d);",blknumberId++,number);
	printf_debug("blknumber tempdata= %s\n", tempdata);
	sqlite3_exec(blknumber_db,tempdata,NULL,NULL,&zErrMsg);
	sqlite3_close(blknumber_db);
}
//删除黑名单从数据库
int sqlite3_blaknumber_del_db(uint32_t number)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("blknum.db",&blknumber_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(blknumber_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(blknumber_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"delete from blknumber where number = %d;",number);
	printf_debug("blknumber tempdata= %s\n", tempdata);
	sqlite3_exec(blknumber_db,tempdata,NULL,NULL,&zErrMsg);
	sqlite3_close(blknumber_db);
}

//清空黑名单数据库
int sqlite3_blaknumber_clr_db(void)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("blknum.db",&blknumber_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(blknumber_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(blknumber_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"delete from blknumber;");
	printf_debug("blknumber tempdata= %s\n", tempdata);
	sqlite3_exec(blknumber_db,tempdata,NULL,NULL,&zErrMsg);
	
	sqlite3_close(blknumber_db);
}

//如果数据库存在打开数据库，不存在就创建数据库
int sqlite3_consume_open_db(void)
{
  	int len;
	 
	/* 打开或创建未采记录数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named recod_db successfully!\n");
	
	/* 创建未采记录表 */
	//recordIdb 一直累加不清零
	//消费时间年/月/日
	char *sql = "create table consume (id int primary key,tag int,money int,time char,datas char);" ;
	sqlite3_exec(recod_db,sql,NULL,NULL,&zErrMsg);
	//sqlite3_close(recod_db);
	return len;
}

//插入数据
int sqlite3_consume_insert_db(struct sRecordStruct sRt)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	   return len;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	/*插入数据	*/
	printf_debug("sRt.CurrentConsumMoney==%d\n",sRt.CurrentConsumMoney);

	sprintf(tempdata,"insert into consume values(%d,%d,%d,%d,'%s');",sRt.recordId,sRt.recordTag, sRt.CurrentConsumMoney ,sRt.ConsumeTime,sRt.recoedDatas);
	printf_debug("tempdata= %s\n", tempdata);
	sqlite3_exec(recod_db,tempdata,NULL,NULL,&zErrMsg);
	sqlite3_close(recod_db);
}

//查询交易记录
int sqlite3_consume_query_record_db( int number)
{
	int len;
	char tempdata[200];
	const char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	}
	
	// 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// -1代表系统会自动计算SQL语句的长度
	sprintf(tempdata,"select *from consume where id = %d;",number);
	printf_debug("tempdata = %s\r\n",tempdata);

	if (sqlite3_prepare_v2(recod_db, tempdata, -1, &stmt, NULL) == SQLITE_OK) 
	{	
		while(sqlite3_step(stmt) == SQLITE_ROW ) 
		{
			// 取出第0列字段的值
			recordSt.recordId = sqlite3_column_int(stmt, 0);
			// 取出第1列字段的值
			recordSt.recordTag = sqlite3_column_int(stmt, 1);
			// 取出第2列字段的值
			recordSt.CurrentConsumMoney = sqlite3_column_int(stmt, 2);
			// 取出第2列字段的值
			recordSt.ConsumeTime = sqlite3_column_int(stmt, 3);
			// 取出第4列字段的值
			buf = sqlite3_column_text(stmt, 4);
			memcpy(recordSt.recoedDatas,buf,64);
			printf_debug("recordId = %d; recordTga = %d; CurrentConsumMoney = %d, ConsumeTime = %d,recordDatas = %s \r\n",recordSt.recordId,recordSt.recordTag,recordSt.CurrentConsumMoney,recordSt.ConsumeTime,recordSt.recoedDatas);			
			break;
		}
	}
	else
	{
		printf_debug("select *from ConsumeData where recordTag > 0 err \r\n");
	}
	sqlite3_finalize(stmt);	
	sqlite3_close(recod_db);
}

//把已采记录的recordTag由0改写成100
int sqlite3_consume_move_db(int index,int number)
{
	char i=0,tempdata[200];
	int len;
	struct sRecordStruct sRt;
	sqlite3_stmt *stmt = NULL; // 用来取数据的
	sqlite3_stmt *stmt1 = NULL; // 用来取数据的
	
	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	//根据ID号从数据库中查找到要删除的记录
	
	//修改本条记录的recordTag
	for(i=0;i<number;i++)
	{
		sprintf(tempdata,"update consume set tag = 1 where id = %d ",index+i);
		//sprintf(tempdata,"select *from ConsumeData where recordId = %d;",index+i);
		printf_debug("tempdata= %s\n", tempdata);
		//sqlite3_exec(recod_db,tempdata,NULL,NULL,&zErrMsg);
		if(sqlite3_exec(recod_db, tempdata, NULL, NULL, &zErrMsg ) != SQLITE_OK)
		{
			printf_debug("zErrMsg=%s\n", zErrMsg);
		}
		else
		{
			printf_debug("Update done.\n");	
		}
	}
	sqlite3_close(recod_db);
}

/****************************************************
bit = 0 查找未采记录指针ID =NoCollectRecordIndex
bit = 1 查找记录最后的ID = SaveRecordIndex
*************************************************/
uint32_t  sqlite3_consume_query_RecordIndex_db(uint8_t bit)
{
	int index =0,indexbak=0,len;
	sqlite3_stmt *stmt = NULL; // 用来取数据的
	bool start =false;

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
		/*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
		sqlite3_close(recod_db);
		return 0;
	}
	else 
		printf("You have opened a sqlite3 database named record successfully!\n");
 
	// 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// -1代表系统会自动计算SQL语句的长度
	if(!bit)//查找未采集记录指针
	{
		char *sql="select *from consume where tag = 0";
		if (sqlite3_prepare_v2(recod_db, sql, -1, &stmt, NULL) == SQLITE_OK) 
		{   
			while(sqlite3_step(stmt) == SQLITE_ROW ) 
			{	
				// 取出第0列字段的值ID号
				index= sqlite3_column_int(stmt, 0);
				sqlite3_finalize(stmt);
				sqlite3_close(recod_db);
				return index ;
			}
			char *sql="select *from consume where tag =0 || tag =1";
			if (sqlite3_prepare_v2(recod_db, sql, -1, &stmt, NULL) == SQLITE_OK) 
			{   
				while(sqlite3_step(stmt) == SQLITE_ROW ) 
				{	
					// 取出第0列字段的值ID号
					index= sqlite3_column_int(stmt, 0);	
				}
			}
			sqlite3_finalize(stmt);
			sqlite3_close(recod_db);
			index++;
			return index;
		}
	}
	else//查找存储记录指针
	{
		char *sql="select *from consume where tag =0 || tag =1";
		if (sqlite3_prepare_v2(recod_db, sql, -1, &stmt, NULL) == SQLITE_OK) 
		{   
		  while(sqlite3_step(stmt) == SQLITE_ROW ) 
		  {	
			// 取出第0列字段的值ID号
			index= sqlite3_column_int(stmt, 0);	
		  }
		}
		sqlite3_finalize(stmt);
		sqlite3_close(recod_db);
		index++;
		return index;
	}	
  return index;
}

//复采交易记录
struct	sRecordStruct sqlite3_consume_query_collectedRecord_db(uint32_t daytime)
{
	// char tempdata[200];
	// char *dTime;
	// const char *buf;
	// int i,len;
	// //struct sRecordStruct sRt ;
	// sqlite3_stmt *stmt = NULL; // 用来取数据的

	// /* 打开数据库 */
	// len = sqlite3_open("record.db",&recod_db);
	// if( len )
	// {
	//    /*fprintf函数格式化输出错误信息到指定的stderr文件流中*/
	//    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	//    sqlite3_close(recod_db);
	//    return recordSt;
	// }
	// else 
	// 	printf("You have opened a sqlite3 database named record successfully!\n");
	
	// recordSt.recordId = 0;
	// recordSt.recordTag =0;
	// recordSt.CurrentConsumMoney = 0;
	// // 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// // -1代表系统会自动计算SQL语句的长度
	// sprintf(tempdata,"select *from consume where time = %d;",daytime);
	// if (sqlite3_prepare_v2(recod_db, tempdata, -1, &stmt, NULL) == SQLITE_OK) 
	// {   
	// 	while(sqlite3_step(stmt) == SQLITE_ROW ) 
	// 	{	
	// 		// 取出第0列字段的值
	// 		recordSt.recordId = sqlite3_column_int(stmt, 0);
	// 		// 取出第1列字段的值
	// 		recordSt.recordTag = sqlite3_column_text(stmt, 1);
	// 		// 取出第2列字段的值
	// 		recordSt.CurrentConsumMoney = sqlite3_column_int(stmt, 2);
	// 		// 取出第4列字段的值
	// 		buf = sqlite3_column_text(stmt, 4);
	// 		memcpy(recordSt.recoedDatas,buf,64);
	// 		printf_debug("recordId = %d; recordTga = %s; CurrentConsumMoney = %d; recordDatas = %s \r\n",recordSt.recordId,recordSt.recordTag,recordSt.CurrentConsumMoney,recordSt.recoedDatas);			
	// 		break;
	// 	}
	// 	sqlite3_finalize(stmt);
	// }
	//sqlite3_close(recod_db);
  return recordSt;
}

//查询某日--某日的消费记录总额跟笔数
struct	sRecordMoneyStruct sqlite3_consume_query_daymoney_db(char *daytime1,char *daytime2)
{
	char tempdata[200];
	int i,len;
	uint32_t dTime, day1,day2;
	
	struct	sRecordMoneyStruct RecordStr;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	   RecordStr.RecordToalMoney =0;
	   RecordStr.RecordToalNumber =0;
	   RecordStr.status = 0;//查询失败
	   return RecordStr;
	}
	else 
		printf("You have opened a sqlite3 database named record successfully!\n");
    
	// 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// -1代表系统会自动计算SQL语句的长度
	day1 = atoi(daytime1);
	day2 = atoi(daytime2);
	//printf_debug("daytim1 = %d day2time = %d \n",day1,day2);
	sprintf(tempdata,"select *from consume where time >= %d ",day1);
	printf_debug("tempdata = %s\n",tempdata);
	RecordStr.RecordToalMoney = 0;
	RecordStr.RecordToalNumber = 0;
	if (sqlite3_prepare_v2(recod_db, tempdata, -1, &stmt, NULL) == SQLITE_OK) 
	{   
		while(sqlite3_step(stmt) == SQLITE_ROW ) 
		{	
			// 取出第3列字段的值
			dTime = sqlite3_column_int(stmt, 3);
		//	printf_debug("dTime = %d\n",dTime);
			RecordStr.RecordToalNumber++;
			// 取出第2列字段的值
			RecordStr.RecordToalMoney += sqlite3_column_int(stmt, 2);
		}
		sqlite3_finalize(stmt);
		RecordStr.status = 1;//查询成功
		printf_debug("RecordStr.RecordToalNumber =%d, RecordStr.RecordToalMoney =%d\n",RecordStr.RecordToalNumber,RecordStr.RecordToalMoney);
	}
	//sqlite3_close(recod_db);
  return RecordStr;
}

//查询未采消费总额跟消费笔数
struct	sRecordMoneyStruct sqlite3_consume_query_consumemoney_db(void)
{
	int i,len;
	struct	sRecordMoneyStruct RecordStr;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	   RecordStr.RecordToalMoney =0;
	   RecordStr.RecordToalNumber =0;
	   RecordStr.status = 0;//查询失败
	   return RecordStr;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named record successfully!\n");
    
	// 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// -1代表系统会自动计算SQL语句的长度
	char *sql="select *from consume where tag = 0 ";
	RecordStr.RecordToalMoney = 0;
	RecordStr.RecordToalNumber = 0;
	if (sqlite3_prepare_v2(recod_db, sql, -1, &stmt, NULL) == SQLITE_OK) 
	{   
		while(sqlite3_step(stmt) == SQLITE_ROW ) 
		{	
			RecordStr.RecordToalNumber++;
			// 取出第2列字段的值
			RecordStr.RecordToalMoney += sqlite3_column_int(stmt, 2);
		}
		RecordStr.status = 1;//查询成功
		printf_debug("RecordStr.RecordToalNumber =%d, RecordStr.RecordToalMoney =%d\n",RecordStr.RecordToalNumber,RecordStr.RecordToalMoney);
	}
	sqlite3_finalize(stmt);
	//sqlite3_close(recod_db);
  	return RecordStr;
}

//查询已采消费总额跟消费笔数
struct	sRecordMoneyStruct sqlite3_consume_query_collectedConsumemoney_db(void)
{
	int i,len;
	struct	sRecordMoneyStruct RecordStr;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	   RecordStr.RecordToalMoney =0;
	   RecordStr.RecordToalNumber =0;
	   RecordStr.status = 0;//查询失败
	   return RecordStr;
	}
	else 
		printf("You have opened a sqlite3 database named recod_db successfully!\n");

	// 每调一次sqlite3_step()函数，stmt就会指向下一条记录
	// -1代表系统会自动计算SQL语句的长度
	char *sql="select *from collecte consume where tag = 1 ";
	RecordStr.RecordToalMoney = 0;
	RecordStr.RecordToalNumber = 0;
	if (sqlite3_prepare_v2(recod_db, sql, -1, &stmt, NULL) == SQLITE_OK) 
	{   
		while(sqlite3_step(stmt) == SQLITE_ROW ) 
		{	
			RecordStr.RecordToalNumber++;
			// 取出第2列字段的值
			RecordStr.RecordToalMoney += sqlite3_column_int(stmt, 2);
		}
		RecordStr.status = 1;//查询成功
		printf_debug("RecordStr.RecordToalNumber =%d, RecordStr.RecordToalMoney =%d\n",RecordStr.RecordToalNumber,RecordStr.RecordToalMoney);
	}
	//sqlite3_close(recod_db);
   	return RecordStr;
}

//删除记录
int sqlite3_consume_delete_db(struct sRecordStruct sRt)
{
	char tempdata[200];
	int len;

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	}

	/* 删除某个特定的数据 */
	sprintf(tempdata,"delete from consume where money = %d and time = %s;",sRt.CurrentConsumMoney,sRt.ConsumeTime);

	printf_debug("tempdata= %s\n", tempdata);
	sqlite3_exec( recod_db , tempdata , NULL , NULL , &zErrMsg );
	//sqlite3_close(recod_db);
}

//清空存储记录数据库
void sqlite3_consume_clr_db(void)
{
	char tempdata[200];
	int len;
	char *buf;
	sqlite3_stmt *stmt = NULL; // 用来取数据的

	/* 打开数据库 */
	len = sqlite3_open("record.db",&recod_db);
	if( len )
	{
	   /*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	   sqlite3_close(recod_db);
	   return;
	  // exit(1);
	}
	else 
		printf("You have opened a sqlite3 database named user successfully!\n");

	sprintf(tempdata,"delete from consume;");
	printf_debug("record tempdata= %s\n", tempdata);
	sqlite3_exec(recod_db,tempdata,NULL,NULL,&zErrMsg);
	
	sqlite3_close(recod_db);
}

void sqlite3_test_gzf()
{
	sqlite3 *db = NULL;
	char *zErrMsg = 0;
	char sql[1024];
	int rc,ID, updata;
	uint8_t i, data[32];
	sqlite3_stmt *stmt;

	rc = sqlite3_open("test.db", &db);
	if (rc) {
		printf_debug("Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	sprintf(sql, "CREATE TABLE IF NOT EXISTS RecordFile(ID INTEGER,isUpdate INTEGER,data BLOB)");
	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
	if (rc)
		printf_debug("create data %s\n", zErrMsg);

	sprintf(sql, "create index ID on RecordFile(ID);");
	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
	if (rc)
		printf_debug("create data %s\n", zErrMsg);

	for(i=0;i<100;i++)
	{
		ID =i;
		updata =100;
		memset(data,11,32);	
		sprintf(sql, "INSERT INTO RecordFile VALUES(%d,%d,? );", ID, updata);

		rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
		printf_debug("sql =%s\n",sql);

		if (rc != SQLITE_OK) {
			fprintf(stderr, "sql error:%s\n", sqlite3_errmsg(db));
		}
		sqlite3_bind_blob(stmt, 1, data, 32, NULL);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
}

//sqlite3数据库测试
void sqlite3_test(void)
{
	// int len,rc;
	// int i=0;
	// int nrow=0;
	// int ncolumn = 0;
	// char *zErrMsg =NULL;
	// char **azResult=NULL; //二维数组存放结果
	// char tempdata[200];
	// char *buf;
	// char sql[1024];

	// struct sRecordStruct sRt;

	// sqlite3_stmt *stmt = NULL; // 用来取数据的
	
	// /* 打开数据库 */
	// len = sqlite3_open("record.db",&recod_db);
	// if( len )
	// {
	// 	/*  fprintf函数格式化输出错误信息到指定的stderr文件流中  */
	// 	fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(recod_db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
	// 	sqlite3_close(recod_db);
	// }
	// else printf("You have opened a sqlite3 database named user successfully!\n");
	// /* 创建未采记录表 */
	// //recordIdb 一直累加不清零
	// //消费时间年/月/日
	// sprintf(sql,"create table consume (id INTEGER,tag INTEGER,money INTEGER,time BLOB,datas BLOB);") ;
	// //char *sql = "create table student (id int primary key,tag int,money int,time char);" ;

	// printf_debug("sql =%s\n",sql);
	// sqlite3_exec(recod_db,sql,NULL,NULL,&zErrMsg);
	
	// for(i=0;i<1000;i++)
	// {
	// 	len = sqlite3_open("record.db",&recod_db);
	// 	//插入数据
	// 	sRt.recordId =i;
	// 	sRt.CurrentConsumMoney =100;
	// 	sRt.recordTag = 0;
	// 	memset(&sRt.ConsumeTime,0x30,6);
	// 	memset(sRt.recoedDatas,0x30,62);

	// 	/*sprintf(tempdata,"insert into consume values (%d,%d,%d,'201202','%s');",sRt.recordId,sRt.recordTag, sRt.CurrentConsumMoney,sRt.recoedDatas);
	// 	printf_debug("%s\n", tempdata);
	// 	sqlite3_exec(recod_db,tempdata,NULL,NULL,&zErrMsg);*/

	// 	sprintf(sql, "INSERT INTO consume VALUES(%d,%d,%d,? ,?);", sRt.recordId,sRt.recordTag, sRt.CurrentConsumMoney);
	// 	printf_debug("sql= %s\n", sql);
	// 	rc = sqlite3_prepare(recod_db, sql, strlen(sql), &stmt, NULL);
	// 	printf_debug("sqlite3_prepare\n");
	// 	if (rc != SQLITE_OK) {
	// 		fprintf(stderr, "sql error:%s\n", sqlite3_errmsg(recod_db));
	// 	}
	// 	sqlite3_bind_blob(stmt, 1, sRt.ConsumeTime, 6, NULL);
	// 	printf_debug("sqlite3_bind_blob\n");
	// //	sqlite3_bind_blob(stmt, 2, sRt.recoedDatas, 62, NULL);
	// 	sqlite3_step(stmt);
	// 	printf_debug("sqlite3_step\n");
	// 	sqlite3_finalize(stmt);
		
	// 	sqlite3_close(recod_db);
	// }

	

	//消费时间年/月/日
//	 char *sql = "create table student; (id int primary key,name char,age int,sex char);";

//	printf_debug("sql =%s\n",sql);
//	sqlite3_exec(recod_db,sql,NULL,NULL,&zErrMsg);
//	
//	for(i=0;i<1000;i++)
//	{
//		//插入数据
//		sRt.recordId =i;
//		sRt.CurrentConsumMoney =100;
//		sRt.recordTag = 0;
//		memset(sRt.ConsumeTime,0x30,6);
//		memset(sRt.recoedDatas,0x30,64);

//		sprintf(tempdata,"insert into student values (%d,'zhang0',20,'m'); ",i);
//		printf_debug("%s\n", tempdata);
//		sqlite3_exec(recod_db,tempdata,NULL,NULL,&zErrMsg);
//	}



//	/* 查询数据 */
//	char *sql11="select *from ConsumeData";
//	sqlite3_get_table( recod_db , sql11 , &azResult , &nrow , &ncolumn , &zErrMsg );
//	printf("nrow=%d ncolumn=%d\n",nrow,ncolumn);
//	printf("the result is:\n");
//	for(i=0;i<(nrow+1)*ncolumn;i++)
//	{
//		printf("azResult[%d]=%s\n",i,azResult[i]);
//	}

//	printf_debug("删除记录\n");
//	/* 删除某个特定的数据 */
//	char *sql111="delete from SensorData where SensorID = 1 ;";
//	sqlite3_exec( recod_db , sql111 , NULL , NULL , &zErrMsg );
//	#ifdef _DEBUG_
//	printf("zErrMsg = %s \n", zErrMsg);
//	sqlite3_free(zErrMsg);
//	#endif

	/* 查询删除后的数据 */
	// char *sq2 = "SELECT * FROM SensorData ";
	// sqlite3_get_table( recod_db , sq2 , &azResult , &nrow , &ncolumn , &zErrMsg );
	// printf( "row:%d column=%d\n " , nrow , ncolumn );
	// printf( "After deleting , the result is : \n" );
	// for( i=0 ; i<( nrow + 1 ) * ncolumn ; i++ )
	// {
	// 	printf( "azResult[%d] = %s\n", i , azResult[i] );
	// }
	// sqlite3_free_table(azResult);
	// #ifdef _DEBUG_
	// printf("zErrMsg = %s \n", zErrMsg);
	// sqlite3_free(zErrMsg);
	// #endif

	// sqlite3_close(recod_db);

}

int sqlite_open_test()
{
	int food_id = 0;
	char context[100] = "西红柿鸡蛋";
	qtdata_t mSndMsg;
}
