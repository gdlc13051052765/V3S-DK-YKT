#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


//消费模式
typedef enum
{
	CONSUM_MONEY = 0,	//计算模式
	CONSUM_RATION = 1,	//自动模式
	CONSUM_PLAN =2,	//价格方案模式
	CONSUM_MENU =3,		//菜单模式
	CONSUM_NUM = 4,//菜号
}_CONSUME_MODE;

/*==================================================================================
* 函 数 名： read_consume_data_form_config_db
* 参    数： 
* 功能描述:  从配置数据库读取消费数据
* 返 回 值： None
* 备    注： 创建成功返回0
* 作    者： lc
* 创建时间： 2021-09-27
==================================================================================*/
int read_consume_data_form_config_db(void);

/*=======================================================================================
* 函 数 名： consume_money_task
* 参    数： None
* 功能描述:  计算模式 CONSUM_MONEY
* 返 回 值： 
* 备    注： 
* 作    者： lc
* 创建时间： 2021-04-20 
==================================================================================*/
void pos_consume_task(void);
