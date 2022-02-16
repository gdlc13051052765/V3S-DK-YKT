#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mydefine.h"
#include "VariableDef.h"
#include "sqlite3.h"
#include "sqliteTask.h"
#include "debug_print.h"
#include "rewrCardTask.h"
#include "msgTask.h"
#include "cJSON.h"


static int consume_mode = CONSUM_MONEY;//消费模式，默认计算模式

static  C_DevMsgSt conDevMsgSt;//设备信息
/*==================================================================================
* 函 数 名： read_consume_data_form_config_db
* 参    数： 
* 功能描述:  从配置数据库读取消费数据
* 返 回 值： None
* 备    注： 创建成功返回0
* 作    者： lc
* 创建时间： 2021-09-27
==================================================================================*/
int read_consume_data_form_config_db(void)
{
	conDevMsgSt = sqlite_read_devMsg_from_config_db();
	printf_debug("限额 = %d\n",conDevMsgSt.dayLimetMoney);  
	printf_debug("卡折扣 = %d\n",conDevMsgSt.cardRebate);
}

/********************************************************
//卡余额诊断
********************************************************/
static uint8_t CardBalanceDiag(void)
{
	uint32_t iii;
	uint32_t jjj;

	if(CurrentConsumMoney > 999999)//单次规划限额9999.99
		return	CARD_LITTLEMONEY;

	iii=ChgBCDStringToUlong(OldBalance, 4);
	jjj=ChgBCDStringToUlong(CardMinBalance, 3);
	jjj+=CurrentConsumMoney;
	
	if(jjj>iii)        //余额不足
		return	CARD_LITTLEMONEY;

	iii=Max_ConsumMoney;//单笔限额
	if(CurrentConsumMoney>iii)
		return	CARD_CONSUM_OVER;

	jjj=Limit_DayMoney;//日限额
	if(CurrentConsumMoney>jjj)
		return	CARD_CONSUM_OVER;

	iii=CardDayConsumMoney;//日累计
	if(Limit_MoneySign>1)//日限额标志
	{
		if(!memcmp(CardConsumDate, SysTimeDatas.TimeString, 3)) 
		{			
			iii += CurrentConsumMoney;
			if(iii>jjj)
				return	CARD_CONSUM_OVER;//超出消费限额
		}
	}
	else if(Limit_MoneySign==1)//月限额标志
	{
		if(!memcmp(CardConsumDate, SysTimeDatas.TimeString, 2)) 
		{
			iii += CurrentConsumMoney;
			if(iii>jjj)
				return	CARD_CONSUM_OVER;//超出消费限额
		}
	}
	return	0;
}

/****************************************************
存储消费记录
*****************************************************/
static uint8_t 	app_ykt_save_record(uint8_t bbit)
{
	uint8_t		aaa[4],buffer[6];
	uint8_t 	*tempbuf, *ptr;
	uint16_t	ii;
	uint32_t	iii;
	uint32_t	SumMoney;
	struct sRecordStruct sRt;
	
	ptr = malloc(sizeof(char)*64);
	if (!bbit)
		SumMoney=SumConsumMoney;
	else
		SumMoney=CurrentConsumMoney;

	if (!bbit)
		ptr[1]=0;//0==消费记录 1==存款记录 2== 补贴记录
	else
	{
		if(WriteCardSta==1)
			return	1;
		else if(WriteCardSta)
			ptr[1]=0x80;//异常记录
	}	
	//sRt.recordTag = malloc(sizeof(char)*4);
	sRt.recordTag = 0;

	ptr[0]=0xa0;	
	if (ConsumMode==CONSUM_NUM)
	{
		if (MenuSort>70)
			MenuSort=0;	
		ptr[2]=MenuSort;
	}
	else if (ConsumMode==CONSUM_RATION)
		ptr[2]=0;//
	else
		ptr[2]=0xfe;//
	memcpy(ptr+4, CardSerialNumBak, 4);//
	memcpy(ptr+8, CardPrinterNum+1, 3);//

	if(SumMoney)//
	{
	    ptr[3]=PurseUsingNum;//钱包号

		memcpy(ptr+11,OldBalance,4);//
		ChgUlongToBCDString(SumMoney,aaa,4);
		sRt.CurrentConsumMoney = SumMoney;
		memcpy(ptr+15,aaa+1,3);//消费额

		iii=ChgBCDStringToUlong(OldBalance,4);
		iii-=SumMoney;
		ChgUlongToBCDString(iii,ptr+18,4);//

		ii=GetU16_HiLo(PurseContrlNum)+1;
		ii = DoubleBigToSmall(ii);
		memcpy(ptr+26,(uchar *)&ii,2);//钱包流水号
		PosConsumCount++;
		PosConsumCount = DoubleBigToSmall(PosConsumCount);	//adlc
		memcpy(ptr+28,(uchar *)&PosConsumCount,2);//设备流水号
		PosConsumCount = DoubleBigToSmall(PosConsumCount);	//adlc
	} else {
	    ptr[3]=0;//限次模式
		memset(ptr+11,0,11);
		memset(ptr+26,0,4);
	}
	//消费时间
	//sRt.ConsumeTime = malloc(sizeof(char)*12);
//	timestruct.S_Time.YearChar   = gdHexToBCD(timestruct.S_Time.YearChar);
//	timestruct.S_Time.MonthChar  = gdHexToBCD(timestruct.S_Time.MonthChar);
//	timestruct.S_Time.DayChar    = gdHexToBCD(timestruct.S_Time.DayChar);
//	timestruct.S_Time.HourChar   = gdHexToBCD(timestruct.S_Time.HourChar);
//	timestruct.S_Time.MinuteChar = gdHexToBCD(timestruct.S_Time.MinuteChar);
//	timestruct.S_Time.SecondChar
	SysTimeDatas = lib_systime_get_systime();
	memcpy(CardConsumTime, SysTimeDatas.TimeString, 6);
	
	HexGroupToHexString(CardConsumTime, buffer, 3);
	sRt.ConsumeTime = atoi(buffer);
	ChgTimeToRecordDatas(CardConsumTime, ptr+22);
	printf_debug("CardConsumTime =%2X %2X %2X %2X", ptr[23],ptr[24],ptr[25],ptr[26]);
	
	ptr[30]=0xff;
	ptr[31]=CalCheckSum(ptr+1,30);
	tempbuf = malloc(sizeof(char)*64);
	//sRt.recoedDatas =malloc(sizeof(char)*80);
	HexGroupToHexString(ptr,sRt.recoedDatas,32);
	printf_debug("sRt.recoedDatas=%s\r\n",sRt.recoedDatas);

	sRt.recordId = SaveRecordIndex;//存储记录指针
	SaveRecordIndex++;

	sqlite3_consume_insert_db(sRt);//插入消费记录到数据库
	free(tempbuf);
	free(ptr);
	return	0;
}

/*=======================================================================================
* 函 数 名： money_mode_consume_task
* 参    数： None
* 功能描述:  计算模式
* 返 回 值： 
* 备    注： 
* 作    者： lc
* 创建时间： 2021-12-02
==================================================================================*/
static void money_mode_consume_task(void)
{
	uint8_t status;
	uint8_t Buffer[20];
	uint32_t iii;
	uint32_t pBalance;

	switch (ConsumCase)
	{
		case 0:
		CurrentConsumMoney = 0;//消费额清零
		ConsumCase = 1;//
		break;

		case 1:
		status=RequestCard();//卡复位信息
		if(!status)//有卡
		{
			ConsumCase = 2;
		}
		break;
		
		case 2:
		status=ReadCardCommonDatas();
		printf_debug("status = %d\n", status);
		if(!status)
		{	
			//Card_Rebate = Read_Rebate();//读出折扣值No_Use
			bitCommStatus=0;
			//找消费钱包号，pin校验读出余额
			status = SearchPurseBalnanceDatas(CardIdentity, 0, &PurseUsingNum, &SelectPurseNum);
			pBalance = ChgBCDStringTouint32_t(OldBalance, 4);
			printf_debug("OldBalance = %d\n", pBalance);

			memcpy(CardSerialNumBak, CardSerialNum, 4);
			if(!CurrentConsumMoney)//查询余额
			{	
				//构建json结构体
				cJSON *root; 
				char *out;  
				root=cJSON_CreateObject();     
				cJSON_AddStringToObject(root, "cmd", "getBalance");  
				cJSON_AddStringToObject(root, "result", "ok"); //查询成功
				HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
				cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
				cJSON_AddStringToObject(root, "consumeStatus", "查询成功"); //查询结果 
				cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
				cJSON_AddNumberToObject(root, "balance", pBalance);//余额

				out =cJSON_Print(root);
				printf("%s\n", out); 
				cJSON_Delete(root);
				send_data_to_qt_display(out);//发送查询结果到QT显示	
				free(out);	

				ConsumCase = 5;//等待拔卡
			} else {//消费模式
				ConsumCase = 3;
			}
		} else {//查询错误
			ConsumCase = 5;//等待取卡
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "getBalance");   
			HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
			cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
			cJSON_AddStringToObject(root, "result", "err"); //卡信息错误
			switch(status)
			{	
				case CARD_BATCH_ERROR:		//批次
				cJSON_AddStringToObject(root, "consumeStatus", "批次错误"); //查询结果 
				break;
				case CARD_NUMBER_ERROR:		//卡号不合法(1.BCD 2.超范围)
				cJSON_AddStringToObject(root, "consumeStatus", "卡号不合法"); //查询结果 
				break;
				case CARD_LOSTCARDNUM:		//挂失卡
				cJSON_AddStringToObject(root, "consumeStatus", "挂失卡"); //查询结果 
				break;
				case CARD_DATA_ERROR:		//用户数据错误(1.卡数据校验和错2.卡折扣为0)
				cJSON_AddStringToObject(root, "consumeStatus", "用户数据错误"); //查询结果 
				break;
				case CARD_LITTLEMONEY:		//金额不足
				cJSON_AddStringToObject(root, "consumeStatus", "金额不足"); //查询结果 
				break;
				case CARD_OVERDATE:			//1.卡过期2.卡未启用3.黑名单有效月4.钱包有效期5.日期格式
				cJSON_AddStringToObject(root, "consumeStatus", "卡过期"); //查询结果 
				break;
			}
			cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字

			out =cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);	
		}
		break;

		case 3: //
		status=CardBalanceDiag();
		if(status)//卡的余额诊断失败
		{
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "consumeResult");   
			cJSON_AddStringToObject(root, "result", "err");//
			HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
			cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
			cJSON_AddStringToObject(root, "consumeStatus", "超过消费限额"); //查询结果 
			cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
			cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
			cJSON_AddNumberToObject(root, "balance", iii);//余额

			out = cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);
			printf("卡诊断失败=%d",status);
			ConsumCase = 5;//等待取卡

		} else {//开始扣款操作
			//hw_pcf8563_get_sysdate(SysTimeDatas.TimeString);
			printf("SysTimeDatas.TimeString == %2x %2x %2x %2x %2x %2x\n",
				SysTimeDatas.TimeString[0], SysTimeDatas.TimeString[1], SysTimeDatas.TimeString[2],
				SysTimeDatas.TimeString[3], SysTimeDatas.TimeString[4], SysTimeDatas.TimeString[5]);
			status = ConsumPocess();//消费
			if(!status)
			{	
				iii = ChgBCDStringTouint32_t(NewBalance, 4);
				SumConsumMoney=CurrentConsumMoney;
				printf("NewBalance = %d\n", iii);
				app_ykt_save_record(0);	//存储消费记录	

				//构建json结构体
				cJSON *root; 
				char *out;  
				root=cJSON_CreateObject();     
				cJSON_AddStringToObject(root, "cmd", "consumeResult");  
				cJSON_AddStringToObject(root, "result", "ok"); 
				HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
				cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
				cJSON_AddStringToObject(root, "consumeStatus", "消费成功"); //查询结果 
				cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
				cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
				cJSON_AddNumberToObject(root, "balance", iii);//余额

				out = cJSON_Print(root);
				printf("%s\n", out); 
				cJSON_Delete(root);
				send_data_to_qt_display(out);//发送查询结果到QT显示	
				free(out);		
				
				ConsumCase=5;//等待拔卡
			} else {
				ConsumCase=4;//等待拔卡
				//构建json结构体
				cJSON *root; 
				char *out;  
				root=cJSON_CreateObject();     
				cJSON_AddStringToObject(root, "cmd", "consumeResult"); 
				cJSON_AddStringToObject(root, "result", "err");  
				HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
				cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
				cJSON_AddStringToObject(root, "consumeStatus", "消费失败"); //查询结果 
				cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
				cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
				cJSON_AddNumberToObject(root, "balance", iii);//余额

				out = cJSON_Print(root);
				printf("%s\n", out); 
				cJSON_Delete(root);
				send_data_to_qt_display(out);//发送查询结果到QT显示	
				free(out);
				printf("扣款失败重新放卡\n");
			}
		}	
		break;

		case 4: //扣款失败重新放卡
		status=CheckCardPrinterNum();
		if(!status)
		{
			status=ReWriteCardSub_M1(0);		
		}
		else
			ConsumCase = 0;//新卡进场重新进入消费流程
		if(!status)
		{
			iii = ChgBCDStringTouint32_t(NewBalance,4);
			SumConsumMoney=CurrentConsumMoney;
			printf("NewBalance = %d\n",iii);
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "consumeResult");  
			cJSON_AddStringToObject(root, "result", "ok"); 
			HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
			cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
			cJSON_AddStringToObject(root, "consumeStatus", "消费成功"); //查询结果 
			cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
			cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
			cJSON_AddNumberToObject(root, "balance", iii);//余额

			out = cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);		
			app_ykt_save_record(0);	//存储消费记录			
			ConsumCase = 5;//等待拔卡	
		} else {	
			ConsumCase = 5;//等待拔卡
		}
		break;

		case 5: //等待卡拿开
		if(!RequestCard())//寻到卡
		{			
			//连续刷卡测试
			#ifdef CON_SWIPE_CARD
			#endif	
		} else {
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "removeCard");   
		
			out = cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);
			printf("卡片已离场\n");
			ConsumCase = 0;//
			CurrentConsumMoney = 0;
		}
		break;

		default:
		break;
	}
}

/*=======================================================================================
* 函 数 名： auto_mode_consume_task
* 参    数： None
* 功能描述:  自动模式
* 返 回 值： 
* 备    注： 
* 作    者： lc
* 创建时间： 2021-12-02
==================================================================================*/
static void auto_mode_consume_task(void)
{
	uint8_t status;
	uint8_t Buffer[20];
	uint32_t iii;
	uint32_t pBalance;

	switch (ConsumCase)
	{
		case 0:
		CurrentConsumMoney = 0;//消费额清零
		ConsumCase = 1;//
		break;

		case 1:
		status=RequestCard();//卡复位信息
		if(!status)//有卡
		{
			ConsumCase = 2;
		}
		break;
		
		case 2:
		status=ReadCardCommonDatas();
		printf_debug("status = %d\n", status);
		if(!status)
		{	
			//Card_Rebate = Read_Rebate();//读出折扣值No_Use
			bitCommStatus=0;
			//找消费钱包号，pin校验读出余额
			status = SearchPurseBalnanceDatas(CardIdentity, 0, &PurseUsingNum, &SelectPurseNum);
			pBalance = ChgBCDStringTouint32_t(OldBalance, 4);
			printf_debug("OldBalance = %d\n", pBalance);

			memcpy(CardSerialNumBak, CardSerialNum, 4);
			if(!CurrentConsumMoney)//查询余额
			{	
				//构建json结构体
				cJSON *root; 
				char *out;  
				root=cJSON_CreateObject();     
				cJSON_AddStringToObject(root, "cmd", "getBalance");  
				cJSON_AddStringToObject(root, "result", "ok"); //查询成功
				HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
				cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
				cJSON_AddStringToObject(root, "consumeStatus", "查询成功"); //查询结果 
				cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
				cJSON_AddNumberToObject(root, "balance", pBalance);//余额

				out =cJSON_Print(root);
				printf("%s\n", out); 
				cJSON_Delete(root);
				send_data_to_qt_display(out);//发送查询结果到QT显示	
				free(out);	

				ConsumCase = 5;//等待拔卡
			} else {//消费模式
				ConsumCase = 3;
			}
		} else {//查询错误
			ConsumCase = 5;//等待取卡
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "getBalance");   
			HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
			cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
			cJSON_AddStringToObject(root, "result", "err"); //卡信息错误
			switch(status)
			{	
				case CARD_BATCH_ERROR:		//批次
				cJSON_AddStringToObject(root, "consumeStatus", "批次错误"); //查询结果 
				break;
				case CARD_NUMBER_ERROR:		//卡号不合法(1.BCD 2.超范围)
				cJSON_AddStringToObject(root, "consumeStatus", "卡号不合法"); //查询结果 
				break;
				case CARD_LOSTCARDNUM:		//挂失卡
				cJSON_AddStringToObject(root, "consumeStatus", "挂失卡"); //查询结果 
				break;
				case CARD_DATA_ERROR:		//用户数据错误(1.卡数据校验和错2.卡折扣为0)
				cJSON_AddStringToObject(root, "consumeStatus", "用户数据错误"); //查询结果 
				break;
				case CARD_LITTLEMONEY:		//金额不足
				cJSON_AddStringToObject(root, "consumeStatus", "金额不足"); //查询结果 
				break;
				case CARD_OVERDATE:			//1.卡过期2.卡未启用3.黑名单有效月4.钱包有效期5.日期格式
				cJSON_AddStringToObject(root, "consumeStatus", "卡过期"); //查询结果 
				break;
			}
			cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字

			out =cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);	
		}
		break;

		case 3: //
		status=CardBalanceDiag();
		if(status)//卡的余额诊断失败
		{
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "consumeResult");   
			cJSON_AddStringToObject(root, "result", "err");//
			HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
			cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
			cJSON_AddStringToObject(root, "consumeStatus", "超过消费限额"); //查询结果 
			cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
			cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
			cJSON_AddNumberToObject(root, "balance", iii);//余额

			out = cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);
			printf("卡诊断失败=%d",status);
			ConsumCase = 5;//等待取卡

		} else {//开始扣款操作
			//hw_pcf8563_get_sysdate(SysTimeDatas.TimeString);
			printf("SysTimeDatas.TimeString == %2x %2x %2x %2x %2x %2x\n",
				SysTimeDatas.TimeString[0], SysTimeDatas.TimeString[1], SysTimeDatas.TimeString[2],
				SysTimeDatas.TimeString[3], SysTimeDatas.TimeString[4], SysTimeDatas.TimeString[5]);
			status = ConsumPocess();//消费
			if(!status)
			{	
				iii = ChgBCDStringTouint32_t(NewBalance, 4);
				SumConsumMoney=CurrentConsumMoney;
				printf("NewBalance = %d\n", iii);
				app_ykt_save_record(0);	//存储消费记录	

				//构建json结构体
				cJSON *root; 
				char *out;  
				root=cJSON_CreateObject();     
				cJSON_AddStringToObject(root, "cmd", "consumeResult");  
				cJSON_AddStringToObject(root, "result", "ok"); 
				HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
				cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
				cJSON_AddStringToObject(root, "consumeStatus", "消费成功"); //查询结果 
				cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
				cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
				cJSON_AddNumberToObject(root, "balance", iii);//余额

				out = cJSON_Print(root);
				printf("%s\n", out); 
				cJSON_Delete(root);
				send_data_to_qt_display(out);//发送查询结果到QT显示	
				free(out);		
				
				ConsumCase=5;//等待拔卡
			} else {
				ConsumCase=4;//等待拔卡
				//构建json结构体
				cJSON *root; 
				char *out;  
				root=cJSON_CreateObject();     
				cJSON_AddStringToObject(root, "cmd", "consumeResult"); 
				cJSON_AddStringToObject(root, "result", "err");  
				HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
				cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
				cJSON_AddStringToObject(root, "consumeStatus", "消费失败"); //查询结果 
				cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
				cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
				cJSON_AddNumberToObject(root, "balance", iii);//余额

				out = cJSON_Print(root);
				printf("%s\n", out); 
				cJSON_Delete(root);
				send_data_to_qt_display(out);//发送查询结果到QT显示	
				free(out);
				printf("扣款失败重新放卡\n");
			}
		}	
		break;

		case 4: //扣款失败重新放卡
		status=CheckCardPrinterNum();
		if(!status)
		{
			status=ReWriteCardSub_M1(0);		
		}
		else
			ConsumCase = 0;//新卡进场重新进入消费流程
		if(!status)
		{
			iii = ChgBCDStringTouint32_t(NewBalance,4);
			SumConsumMoney=CurrentConsumMoney;
			printf("NewBalance = %d\n",iii);
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "consumeResult");  
			cJSON_AddStringToObject(root, "result", "ok"); 
			HexGroupToHexString(CardPrinterNum, Buffer, 4);//卡号转16进制字符串
			cJSON_AddStringToObject(root, "cardNum", Buffer);  //卡号
			cJSON_AddStringToObject(root, "consumeStatus", "消费成功"); //查询结果 
			cJSON_AddStringToObject(root, "name", nameBuf);  //用户名字
			cJSON_AddNumberToObject(root, "currentMoney", CurrentConsumMoney);//消费额
			cJSON_AddNumberToObject(root, "balance", iii);//余额

			out = cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);		
			app_ykt_save_record(0);	//存储消费记录			
			ConsumCase = 5;//等待拔卡	
		} else {	
			ConsumCase = 5;//等待拔卡
		}
		break;

		case 5: //等待卡拿开
		if(!RequestCard())//寻到卡
		{			
			//连续刷卡测试
			#ifdef CON_SWIPE_CARD
			#endif	
		} else {
			//构建json结构体
			cJSON *root; 
			char *out;  
			root=cJSON_CreateObject();     
			cJSON_AddStringToObject(root, "cmd", "removeCard");   
		
			out = cJSON_Print(root);
			printf("%s\n", out); 
			cJSON_Delete(root);
			send_data_to_qt_display(out);//发送查询结果到QT显示	
			free(out);
			printf("卡片已离场\n");
			ConsumCase = 0;//
			CurrentConsumMoney = 0;
		}
		break;

		default:
		break;
	}
}

/*===============================================================================
* 函 数 名： pos_consume_task
* 参    数： None
* 功能描述:  消费流程
* 返 回 值： 
* 备    注： 
* 作    者： lc
* 创建时间： 2021-12-01
==================================================================================*/
void pos_consume_task(void)
{
	switch(consume_mode)
	{
		case  CONSUM_MONEY://计算模式
			money_mode_consume_task();
		break;
		case CONSUM_RATION://自动模式
			auto_mode_consume_task();
		break;
	}
}
