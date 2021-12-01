#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


//设备状态
enum {
  C_INIT_STATUS            = 0 ,//生产状态
	C_WAIT_TOPIC_STATUS ,      //证书获取完成等待订阅
	C_FACTORY_BIND_OK_STATUS , //完成出厂绑定（解绑状态）
	C_SITE_BIND_OK_STATUS ,    //现场绑定完成，可以正常写盘拉菜单
	C_LOCAL_UNBIND_STATUS ,    //本地解绑完成，一直上传解绑状态到后台
}cDevDtatus;

//设备信息结构体
typedef struct 
{
  char* version ;//版本号
  int   maincode ;
  char* ipaddr; 
  int   port;
  char* matchCode;
  char* cardKeyCode;
  char* calCardKey;
  int   cardMinBalance;
  int   dayLimetMoney ;
  char* commEncryptKey; 
  char* cardBatchEnable;
  char* purseEnable;
  int   consumeMode ;
  int   cardSector;//公共扇区
  int   cardEnableMonths;//黑名单有效期
}C_DevMsgSt;


//脱机交易记录结构体
struct	sRecordStruct
{
	int  recordId;
	int  recordTag;
	int  CurrentConsumMoney;
	uint32_t  ConsumeTime;
	char recoedDatas[80];		
};


//价格方案结构体
struct	sMoneyplanStruct
{
	int  serid;//卡身份
	char recoedDatas[30];//价格方案		
};

//脱机交易记录结构体
struct	sRecordMoneyStruct
{
	char status;
	int  RecordToalNumber;	
	int  RecordToalMoney;		
};

extern struct sRecordStruct recordSt;


//如果数据库存在打开数据库，不存在就创建数据库
int sqlite3_consume_open_db();
//sqlite3数据库测试
void sqlite3_test();
//插入数据
int sqlite3_consume_insert_db(struct sRecordStruct sRt);
//上传交易记录
int sqlite3_consume_query_record_db(int number);
//关闭数据库
void sqlite3_consume_close_db();
//查询未采消费总额跟消费笔数
struct	sRecordMoneyStruct sqlite3_consume_query_consumemoney_db(void);
//查询已采消费总额跟消费笔数
struct	sRecordMoneyStruct sqlite3_consume_query_collectedConsumemoney_db(void);
/*查询未采集记录初始记录的id=NoCollectRecordIndex
bit = 0 查找记录初始ID
bit = 1 查找记录最后的ID
********************************************/
int  sqlite3_consume_query_tRecordIndex_db(int bit);
//删除记录
int sqlite3_consume_delete_db(struct sRecordStruct sRt);
//从未采记录数据库中把已采记录移动到已采数据库
int sqlite3_consume_move_db(int index,int number);
//查询某日--某日的消费记录总额跟笔数
struct	sRecordMoneyStruct sqlite3_consume_query_daymoney_db(char *daytime1,char *datatime2);

//复采交易记录
struct	sRecordStruct sqlite3_consume_query_collectedRecord_db(uint32_t daytime);
//清空存储记录数据库
void sqlite3_consume_clr_db(void);


//如果黑名单数据库存在打开数据库，不存在就创建数据库
int sqlite3_blaknumber_open_db(void);
//插入黑名单数据库
int sqlite3_blaknumber_insert_db(uint32_t);
//删除黑名单从数据库
int sqlite3_blaknumber_del_db(uint32_t);
//清空黑名单数据库
int sqlite3_blaknumber_clr_db(void);
//查询是否在黑名单数据库
int sqlite3_blaknumber_query_db(uint32_t);
//创建价格方案数据库
int sqlite3_moneyplan_open_db(void);
//插入价格方案到数据库
int sqlite3_moneyplan_insert_db(struct sMoneyplanStruct stru);
//查找符合身份的价格方案
struct sMoneyplanStruct sqlite3_moneyplan_query_db(uint32_t serid );
//查找价格方案存储记录指针
uint32_t  sqlite3_query_moneyplan_db(void);
//清空价格方案数据库
int sqlite3_moneyplan_clr_db(void);
/*==================================================================================
* 函 数 名： sqlite_create_config_db
* 参    数： 
* 功能描述:  创建设备参数配置数据库
* 返 回 值： None
* 备    注： 创建成功返回0
* 作    者： lc
* 创建时间： 2021-05-25
==================================================================================*/
int sqlite_create_config_db();

/*==================================================================================
* 函 数 名： sqlite_read_devMsg_config_db
* 参    数： 
* 功能描述:  从配置数据库读取设备信息
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 
==================================================================================*/
C_DevMsgSt sqlite_read_devMsg_from_config_db();

/*==================================================================================
* 函 数 名： sqlite_update_matchCode_config_db
* 参    数： 
* 功能描述:  修改配置数据库匹配字
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_matchCode_config_db(char *snbuf);

/*==================================================================================
* 函 数 名： sqlite_update_cardKeyCode_config_db
* 参    数： 
* 功能描述:  修改配置数据库读卡密钥
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardKeyCode_config_db(char *snbuf);

/*==================================================================================
* 函 数 名： sqlite_update_cardSector_config_db
* 参    数： 
* 功能描述:  修改配置数据库公共扇区
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardSector_config_db(int cardSector);

/*==================================================================================
* 函 数 名： sqlite_update_CalCardKey_config_db
* 参    数： 
* 功能描述:  修改配置数据库匹配字
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_CalCardKey_config_db(char *CalCardKey);

/*==================================================================================
* 函 数 名： sqlite_update_cardBatchEnable_config_db
* 参    数： 
* 功能描述:  修改配置数据库卡钱包
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardBatchEnable_config_db(char *cardBatchEnable);

/*==================================================================================
* 函 数 名： sqlite_update_cardMinBalance_config_db
* 参    数： 
* 功能描述:  修改配置数据库卡底金
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_cardMinBalance_config_db(int cardMinBalance);

/*==================================================================================
* 函 数 名： sqlite_update_cardMinBalance_config_db
* 参    数： 
* 功能描述:  修改配置数据库日限额
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //
==================================================================================*/
int sqlite_update_dayLimetMoney_config_db(int dayLimetMoney);

/*==================================================================================
* 函 数 名： sqlite_update_commEncryptKey_config_db
* 参    数： 
* 功能描述:  修改配置数据库传输密钥
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 //  
==================================================================================*/
int sqlite_update_commEncryptKey_config_db(char *commEncryptKey);


void malloc_devMsgSt(C_DevMsgSt pdev);
void free_devMsgSt(C_DevMsgSt pdev);


//数据库测试
void sqlite_test(void);
int sqlite_open_test();
