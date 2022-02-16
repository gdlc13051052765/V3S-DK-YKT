#include <stdio.h>  
#include <stdbool.h>  
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqliteTask.h"
#include "udp_data_analyse.h"
#include "debug_print.h"
#include "CalucationFile.h"
#include "sysTime.h"
#include "ExternVariableDef.h"
#include "mydefine.h"
#include "crcFiles.h"
#include "udp_fifo.h"

#define MAX_MUTIL_LEN   200 
#define MAX_MUTIL_ITEM_NUM	10

static send_queue_t	udp_queue;//udp发送队列
unsigned char Tx_Buffer[2048];  //udp数据缓冲区
uint16_t udp_len = 0;
static uint8_t	commEncryptKey[8];//传输密钥
static uint32_t cardMinBalance = 1000;//卡底金
static uint8_t matchCode[4];//匹配字

struct sRecordStruct sRt;//脱机交易记录
struct sRecordStruct sUpRt;//上传脱机交易记录
struct sRecordMoneyStruct RecordStr;//脱机记录总额笔数
struct sRecordStruct recordSt;

static C_DevMsgSt 	pdevMsg;//设备信息

static void udpSendSub(void);

void Init_Serial(void)
{
	SerialReceiveLen=0;
	bitSerial_ReceiveEnd=0;
	SerialReCase=0;
	SerialReceiveTimeCount=0;
	memset(SerialUnion.S_DatasBuffer,0,100);
} 

static void Chg_BlkNameSub(uint8_t bbit, uint8_t *blknum)//增加黑名单
{
	uint32_t iii=0;

	iii=ChgBCDStringToUlong(blknum, 4);
	if(!bbit)//增加黑名单
	{
		sqlite3_blaknumber_insert_db(iii);
	}
	else//从黑名单数据库清除
	{
		sqlite3_blaknumber_del_db(iii);
	}
}

static void CalComdSymblDatas(uint8_t Comd, uint8_t * Datas)
{
	uint8_t  i;

	for (i=0;i<6;i++)
		Datas[i]=Comd+0x11* i;
}

/*==================================================================================
* 函 数 名： read_devMsg_from_config_db
* 参    数： 
* 功能描述:  从配置数据库读取设备信息
* 返 回 值： None
* 备    注： 修改成功返回0
* 作    者： lc
* 创建时间： 2021-05-25 
==================================================================================*/
static uint8_t cardKeyCode[6];//读卡密钥
static uint8_t calCardKey[8];//写卡密钥
static uint8_t cardBatchEnable[32];//卡批次
static uint8_t purseEnable[10];//钱包号
void udp_read_devMsg_from_config_db()
{
	udp_instance_init();

	pdevMsg = sqlite_read_devMsg_from_config_db();
	printf_debug("maincode = %d \n", pdevMsg.maincode);
	printf_debug("MatchCode = %s \n", pdevMsg.matchCode);
	HexStringToHexGroup(pdevMsg.matchCode, matchCode, 4);
	printf_debug("CardKeyCode = %s \n", pdevMsg.cardKeyCode);
	HexStringToHexGroup(pdevMsg.cardKeyCode, cardKeyCode, 6);
	printf_debug("CalCardKey = %s \n", pdevMsg.calCardKey);
	HexStringToHexGroup(pdevMsg.calCardKey, calCardKey, 8);
	printf_debug("CardMinBalance = %d \n", pdevMsg.cardMinBalance);
	cardMinBalance =  pdevMsg.cardMinBalance;
	printf_debug("DayLimetMoney = %d \n", pdevMsg.dayLimetMoney);
	printf_debug("CommEncryptKey = %s \n", pdevMsg.commEncryptKey);
	HexStringToHexGroup(pdevMsg.commEncryptKey,commEncryptKey, 8);
	printf_debug("CardBatchEnable = %s \n", pdevMsg.cardBatchEnable);
	HexStringToHexGroup(pdevMsg.cardBatchEnable,cardBatchEnable, 32);
	printf_debug("PurseEnable = %s \n", pdevMsg.purseEnable);
	HexStringToHexGroup(pdevMsg.purseEnable,purseEnable, 10);
	printf_debug("consumeMode = %d \n", pdevMsg.consumeMode);
	printf_debug("cardEnableMonths = %d \n", pdevMsg.cardEnableMonths);
	printf_debug("CardRebate = %d \n", pdevMsg.cardRebate);

	
}

void ReceiveSub(char *data)//接收数据处理
{
	uint8_t		status,j;
	uint8_t		DatasBuffer[40];
	uint8_t		aaa[6]={0,0,0,0,0,0};
	uint8_t    	buf[1];
	uint8_t   	Buffer[100]={0},tempbuf[512] = {0};
	uint8_t  	LoadSum =0;
	uint8_t		i,PPage,Yearchar,MonthChar,DayChar,Nums;
	uint16_t 	bigtosmalltemp,downflag;
	uint32_t 	ii,Addr,iii,SumMoney;
	struct	sMoneyplanStruct stru;

	printf_debug("udp receive data =");
	HexStringToHexGroup(data+1, SerialUnion.S_DatasBuffer, strlen(data)-4);
	for(i=0; i<SerialUnion.S_DatasStruct.DatasLen+11; i++)
		printf_debug("%2x ", SerialUnion.S_DatasBuffer[i]);
	printf_debug("\n");
	SerialUnion.S_DatasStruct.UartReAddrCode = DoubleBigToSmall(SerialUnion.S_DatasStruct.UartReAddrCode);
	SerialUnion.S_DatasStruct.UartComd = DoubleBigToSmall(SerialUnion.S_DatasStruct.UartComd);
	SerialUnion.S_DatasStruct.UartSeAddrCode = DoubleBigToSmall(SerialUnion.S_DatasStruct.UartSeAddrCode);
	printf_debug("UartReAddrCode = %d\n", SerialUnion.S_DatasStruct.UartReAddrCode);
	printf_debug("pdevMsg.maincode = %d\n", pdevMsg.maincode );
	printf_debug("UartComd = %2x\n", SerialUnion.S_DatasStruct.UartComd);
	printf_debug("UartSeAddrCode = %d\n", SerialUnion.S_DatasStruct.UartSeAddrCode);
	printf_debug("UartSeAddrCode = %d\n", SerialUnion.S_DatasStruct.DatasLen );

	if(pdevMsg.maincode ==SerialUnion.S_DatasStruct.UartReAddrCode |SerialUnion.S_DatasStruct.UartComd==RD_ADDR_COMD)
	{
		printf_debug("udp receive data ok \n");
	} else {
		printf_debug("udp receive data err \n");
		return;
	}
	bitSerial_SendRequest=1;
	SerialUnion.S_DatasStruct.UartStatus=0;
	memset(Buffer,0,100);
	switch (SerialUnion.S_DatasStruct.UartComd & 0xfff)
	{
		printf_debug("SerialUnion.S_DatasStruct.UartComd == %d\n", SerialUnion.S_DatasStruct.UartComd);
		case RD_ADDR_COMD://读站点
			SerialUnion.S_DatasStruct.Datas[0]=pdevMsg.maincode >>8;
			SerialUnion.S_DatasStruct.Datas[1]=(uchar)pdevMsg.maincode ;
			SerialUnion.S_DatasStruct.DatasLen=2;
			break;
		case DOWNLODER_COMD:
			LoadModeFlag = 1;
			LoadModeResponse[0] = STX;
			LoadModeResponse[1] =SerialUnion.S_DatasStruct.UartReAddrCode>>8;
			LoadModeResponse[2] =SerialUnion.S_DatasStruct.UartReAddrCode%256;
			LoadModeResponse[3] =SerialUnion.S_DatasStruct.UartSeAddrCode>>8	;
			LoadModeResponse[4] =SerialUnion.S_DatasStruct.UartSeAddrCode%256;
			LoadModeResponse[5] =SerialUnion.S_DatasStruct.UartComd	;
			LoadModeResponse[6] =SerialUnion.S_DatasStruct.UartStatus;
			LoadModeResponse[7] =SerialUnion.S_DatasStruct.UartAddrH	;
			LoadModeResponse[8] =SerialUnion.S_DatasStruct.UartAddrL	;
			LoadModeResponse[9] =SerialUnion.S_DatasStruct.UartFrameNum;
			LoadModeResponse[10]=2;
			LoadModeResponse[11]='G';
			LoadModeResponse[12]='G';

			for(i=1;i<13;i++)
			{
				LoadSum+=LoadModeResponse[i];
			}
			LoadModeResponse[13]=ETX;
			LoadModeResponse[14]=LoadSum;
			break;
		case RD_USERCODE_COMD://上传匹配字
			memcpy(DatasBuffer,matchCode,4);
			DatasBuffer[4]=CalCheckSum(DatasBuffer,4);
			CalEncryptDatas(0,DatasBuffer,commEncryptKey,SerialUnion.S_DatasStruct.Datas,5);
			SerialUnion.S_DatasStruct.DatasLen=5;
			SerialUnion.S_DatasStruct.UartComd|=0x8000;	
			break;
		case SET_USERCODE_COMD://设置匹配字
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if ((SerialUnion.S_DatasStruct.UartComd & 0x8000) && SerialUnion.S_DatasStruct.DatasLen==5)
			{
				CalEncryptDatas(1,SerialUnion.S_DatasStruct.Datas,commEncryptKey,DatasBuffer+1,5);
				if (!BytesCheckSum(DatasBuffer+1,5))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					DatasBuffer[0]=0xa0;
					DatasBuffer[5]=CalCheckSum(DatasBuffer,5);
					
					HexGroupToHexString(DatasBuffer+1,Buffer,4);
				
					if(memcmp(pdevMsg.matchCode,DatasBuffer+1,4))
					{
						memcpy(pdevMsg.matchCode,DatasBuffer+1,4);
						for(i=0; i<4;i++)
							printf_debug("%2X ",pdevMsg.matchCode[i]);
						sqlite_update_matchCode_config_db(Buffer);
						bitUpdateParameter=1;	
					}
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			SerialUnion.S_DatasStruct.UartComd&=0xfff;	 										
			break;
		case SET_RDCARDCODE_COMD://下载读卡密码
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if ((SerialUnion.S_DatasStruct.UartComd & 0x8000) && SerialUnion.S_DatasStruct.DatasLen==7)
			{	
				//HexStringToHexGroup(commEncryptKey,commEncryptKey,);
				printf_debug("commEncryptKey = ");
				for(i=0;i<8;i++)
					printf_debug(" %02x",commEncryptKey[i]);
				printf_debug("\n");
				CalEncryptDatas(1,SerialUnion.S_DatasStruct.Datas,commEncryptKey,DatasBuffer+1,7);//瑙ｅ瘑
				
				if (!BytesCheckSum(DatasBuffer+1,7))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;	
					HexGroupToHexString(DatasBuffer+1,Buffer,6);
					if(memcmp(pdevMsg.cardKeyCode,DatasBuffer+1,6))
					{
						memcpy(pdevMsg.cardKeyCode,DatasBuffer+1,6);	
						sqlite_update_cardKeyCode_config_db(Buffer);		
						SerialUnion.S_DatasStruct.DatasLen=0;
						bitUpdateParameter=1;	
					}	
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			SerialUnion.S_DatasStruct.UartComd&=0xfff;	 										
			break;
		case RD_RDCARDCODE_COMD://上传读卡密码
			memcpy(DatasBuffer,cardKeyCode,6);
			DatasBuffer[6]=CalCheckSum(DatasBuffer,6);
			CalEncryptDatas(0,DatasBuffer,commEncryptKey,SerialUnion.S_DatasStruct.Datas,7);//鍔犲瘑
			SerialUnion.S_DatasStruct.DatasLen=7;
			SerialUnion.S_DatasStruct.UartComd|=0x8000;	//鏁版嵁鍔犲瘑			
			break;
		case RD_COMMSECTOR_COMD://读出公共扇区号
			SerialUnion.S_DatasStruct.Datas[0]=pdevMsg.cardSector;
			SerialUnion.S_DatasStruct.DatasLen=1;
			break;			
		case SET_COMMSECTOR_COMD://设置公共扇区号
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==1)
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
				if(pdevMsg.cardSector !=SerialUnion.S_DatasStruct.Datas[0])
				{
					pdevMsg.cardSector = SerialUnion.S_DatasStruct.Datas[0];
					sqlite_update_cardSector_config_db(pdevMsg.cardSector);	
					bitUpdateParameter=1;	
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;	
			break;
		case SET_CALCARDKEY_COMD://下载写卡密码
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if ((SerialUnion.S_DatasStruct.UartComd & 0x8000) && SerialUnion.S_DatasStruct.DatasLen==9)
			{
				CalEncryptDatas(1,SerialUnion.S_DatasStruct.Datas,commEncryptKey,DatasBuffer+1,9);//瑙ｅ瘑
				if (!BytesCheckSum(DatasBuffer+1,9))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					//if(memcmp(pdevMsg.calCardKey,DatasBuffer+1,8))
					{
						memcpy(pdevMsg.calCardKey,DatasBuffer+1,8);
						HexGroupToHexString(DatasBuffer+1,Buffer,8);
						memcpy(pdevMsg.calCardKey,DatasBuffer+1,8);		
						sqlite_update_CalCardKey_config_db(Buffer);			
						bitUpdateParameter=1;	
					}
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			SerialUnion.S_DatasStruct.UartComd&=0xfff;
			break;
		case RD_CALCARDKEY_COMD://上传写卡密码
			memcpy(DatasBuffer,calCardKey,8);
			DatasBuffer[8]=CalCheckSum(DatasBuffer,8);
			CalEncryptDatas(0,DatasBuffer,commEncryptKey,SerialUnion.S_DatasStruct.Datas,9);//鍔犲瘑
			SerialUnion.S_DatasStruct.DatasLen=9;
			SerialUnion.S_DatasStruct.UartComd|=0x8000;
			break;

		case POS_RST_COMD:   //POS机复位
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(POS_RST_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;	
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
			
		case SET_BATCH_COMD://设置批次是否有效
			if (SerialUnion.S_DatasStruct.DatasLen==32)
			{
				memcpy(DatasBuffer,SerialUnion.S_DatasStruct.Datas,32);
				if(memcmp(pdevMsg.cardBatchEnable,DatasBuffer,32))
				{
					memcpy(pdevMsg.cardBatchEnable,DatasBuffer,32);
					HexGroupToHexString(DatasBuffer,Buffer,32);
					sqlite_update_cardBatchEnable_config_db(Buffer);
					bitUpdateParameter=1;	
				}
			}
			else
				SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case RD_BATCH_COMD://读出批次
			SerialUnion.S_DatasStruct.DatasLen=32;
			memcpy(SerialUnion.S_DatasStruct.Datas,cardBatchEnable,32);
	   		break;
		case RD_MINMONEY_COMD://上传底金
			SerialUnion.S_DatasStruct.Datas[0]=0;
			SerialUnion.S_DatasStruct.Datas[1] = pdevMsg.cardMinBalance>>16;
			SerialUnion.S_DatasStruct.Datas[2] = pdevMsg.cardMinBalance>>8;
			SerialUnion.S_DatasStruct.Datas[3] = pdevMsg.cardMinBalance;
			//memcpy(SerialUnion.S_DatasStruct.Datas+1,&pdevMsg.cardMinBalance,3);
			SerialUnion.S_DatasStruct.DatasLen=4;			
			break;
		case SET_MINMONEY_COMD://下载底金
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==4 && !BCD_String_Diag(SerialUnion.S_DatasStruct.Datas,4))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
				if(memcmp(&pdevMsg.cardMinBalance,SerialUnion.S_DatasStruct.Datas+1,3))
				{
					//HexGroupToHexString(DatasBuffer+1,SerialUnion.S_DatasStruct.Datas+1,3);
					pdevMsg.cardMinBalance = ChgBCDStringToUlong(SerialUnion.S_DatasStruct.Datas+1,3);
					sqlite_update_cardMinBalance_config_db(pdevMsg.cardMinBalance);
					bitUpdateParameter=1;		
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;			
			break;
		case RD_DAYLIMET_COMD://读取日限额
			SerialUnion.S_DatasStruct.Datas[0]=0;
			memcpy(SerialUnion.S_DatasStruct.Datas+1,&pdevMsg.dayLimetMoney,3);
			SerialUnion.S_DatasStruct.DatasLen=4;			
			break;
		case SET_DAYLIMET_COMD://设置日限额
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==4 && !BCD_String_Diag(SerialUnion.S_DatasStruct.Datas,4))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
				//if(pdevMsg.dayLimetMoney,SerialUnion.S_DatasStruct.Datas+1,3)
				{
					memcpy(&pdevMsg.dayLimetMoney,DatasBuffer+1,3);
					pdevMsg.cardMinBalance = ChgBCDStringToUlong(DatasBuffer+1,3);
					sqlite_update_dayLimetMoney_config_db(pdevMsg.cardMinBalance);
					bitUpdateParameter=1;		
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;			
			break;
		case RD_BLKNAME_TIME_COMD://读取黑名单有效期
			SerialUnion.S_DatasStruct.Datas[0]=pdevMsg.cardEnableMonths;
			SerialUnion.S_DatasStruct.DatasLen=1;
			break;
		case SET_BLKNAMETIME_COMD://设置黑名单有效期
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==1)
			{
				sqlite_update_cardEnableMonths_config_db(SerialUnion.S_DatasStruct.Datas[0]);
				bitUpdateParameter=1;
			}
			SerialUnion.S_DatasStruct.DatasLen=0;		
			break;
		case CLR_PURSE_COMD://读出钱包
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(CLR_PURSE_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case SET_PURSE_COMD://设置钱包
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
			if (SerialUnion.S_DatasStruct.UartAddrL<100 && !(SerialUnion.S_DatasStruct.DatasLen%13))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;	
				if(memcmp(pdevMsg.purseEnable,SerialUnion.S_DatasStruct.Datas,10))
				{
					memcpy(pdevMsg.purseEnable,SerialUnion.S_DatasStruct.Datas,10);
					HexGroupToHexString(SerialUnion.S_DatasStruct.Datas,Buffer,13);
					sqlite_update_cardBatchEnable_config_db(Buffer);
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case RD_TIME2_COMD://读取系统时间
			SysTimeDatas = lib_systime_get_systime();
			memcpy(SerialUnion.S_DatasStruct.Datas,SysTimeDatas.TimeString,7);
			SerialUnion.S_DatasStruct.DatasLen=7;
			break;
		case SET_TIME2_COMD://设置系统时间
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==7 && !DiagTimeString(0,SerialUnion.S_DatasStruct.Datas) && !DiagTimeString(1,SerialUnion.S_DatasStruct.Datas+3))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
			//	hw_pcf8563_set_sysdate(SerialUnion.S_DatasStruct.Datas);
				lib_systime_set_systime(SerialUnion.S_DatasStruct.Datas[0],SerialUnion.S_DatasStruct.Datas[1],SerialUnion.S_DatasStruct.Datas[2],
										SerialUnion.S_DatasStruct.Datas[3],SerialUnion.S_DatasStruct.Datas[4],SerialUnion.S_DatasStruct.Datas[5]);
				sprintf(tempbuf, "date -s \"20%2x-%2x-%2x %2x:%2x:%2x\"", SerialUnion.S_DatasStruct.Datas[0],SerialUnion.S_DatasStruct.Datas[1],SerialUnion.S_DatasStruct.Datas[2],
										SerialUnion.S_DatasStruct.Datas[3],SerialUnion.S_DatasStruct.Datas[4],SerialUnion.S_DatasStruct.Datas[5]);
				printf_debug("设置系统时间 = %s\n", tempbuf);
				system(tempbuf);
				system("hwclock -w");
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case CLR_BLKNUM_COMD://清除黑名单
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(CLR_BLKNUM_COMD,DatasBuffer);
				sqlite3_blaknumber_clr_db();
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case ADD_BLKNUM_COMD://下载黑名单
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
		
			if (SerialUnion.S_DatasStruct.DatasLen && !(SerialUnion.S_DatasStruct.DatasLen%4))
			{
				status=SerialUnion.S_DatasStruct.DatasLen/4;
				for(i=0;i<status;i++)
				{
					//iii=ChgBCDStringToUlong(SerialUnion.S_DatasStruct.Datas+i*4,4);
					//gdHexGroupToHexString(SerialUnion.S_DatasStruct.Datas +i*4,Buffer,4);
					printf_debug("blknumber = %s\n",Buffer);
					Chg_BlkNameSub(0,SerialUnion.S_DatasStruct.Datas +i*4);
				}
				SerialUnion.S_DatasStruct.UartStatus=0;
			}	
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case DEL_BLKNUM_COMD://删除黑名单
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
			if (SerialUnion.S_DatasStruct.DatasLen && !(SerialUnion.S_DatasStruct.DatasLen%4))
			{
				status=SerialUnion.S_DatasStruct.DatasLen/4;
				for(i=0;i<status;i++)
				{
					//iii=ChgBCDStringToUlong(SerialUnion.S_DatasStruct.Datas+i*4,4);
					//gdHexGroupToHexString(SerialUnion.S_DatasStruct.Datas +i*4,Buffer,4);
					Chg_BlkNameSub(1,SerialUnion.S_DatasStruct.Datas +i*4);
				}
				SerialUnion.S_DatasStruct.UartStatus=0;
			}	
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case DEL_ADD_BLKNUM_COMD:
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
			if (SerialUnion.S_DatasStruct.DatasLen && !(SerialUnion.S_DatasStruct.DatasLen%5))
			{
				status=SerialUnion.S_DatasStruct.DatasLen/5;
				for(i=0;i<status;i++)
				{	
					//iii=ChgBCDStringToUlong(SerialUnion.S_DatasStruct.Datas+i*5+1,4);
					HexGroupToHexString(SerialUnion.S_DatasStruct.Datas+i*4, Buffer, 4);
					//if (iii<MAXCARDPRINTERNUM)
					{
						if (SerialUnion.S_DatasStruct.Datas[i*5]==0x55)
							Chg_BlkNameSub(0,Buffer);
						if (SerialUnion.S_DatasStruct.Datas[i*5]==0xaa)
							Chg_BlkNameSub(1,Buffer);
					}	
				}
				SerialUnion.S_DatasStruct.UartStatus=0;
			}	
			SerialUnion.S_DatasStruct.DatasLen=0;						
			break;
		case CLR_POSDATAS_COMD://清除POS机数据
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==7)
			{
				CalComdSymblDatas(CLR_POSDATAS_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas+1,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					/*bitNeedRST=1;
					Disp_Clr_Ram();
					Clr_PosSub(1,SerialUnion.S_DatasStruct.Datas[0]);
					STX_FLAG =1;*/
					sqlite3_consume_clr_db();
					sqlite3_blaknumber_clr_db();					
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case RD_POSSTATUS_COMD://读取POS机状态
			//InitSaveRecordSub(0);
			SerialUnion.S_DatasStruct.Datas[0]=0;
			bigtosmalltemp = DoubleBigToSmall(RecordSum); //adlc
			memcpy(SerialUnion.S_DatasStruct.Datas+1,(uchar *)&bigtosmalltemp,2);
			bigtosmalltemp = DoubleBigToSmall(NoCollectRecordSum); //adlc
			memcpy(SerialUnion.S_DatasStruct.Datas+3,(uchar *)&bigtosmalltemp,2);
			bigtosmalltemp = DoubleBigToSmall(ReCollectRecordIndex);//adlc
			memcpy(SerialUnion.S_DatasStruct.Datas+5,(uchar *)&bigtosmalltemp,2);
			bigtosmalltemp = DoubleBigToSmall(SaveRecordIndex);//adlc
			memcpy(SerialUnion.S_DatasStruct.Datas+7,(uchar *)&bigtosmalltemp,2);
			SerialUnion.S_DatasStruct.DatasLen=9;
			break;
		case SET_ENCRYPTKEY_COMD://下载传输密钥
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==8)
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
				
				if(memcmp(commEncryptKey,SerialUnion.S_DatasStruct.Datas,8))
				{
					memcpy(commEncryptKey,SerialUnion.S_DatasStruct.Datas,8);
					printf_debug("commEncryptKey = ");
					for(i=0; i<8;i++)
					{
						printf_debug("%2X ",commEncryptKey[i]);
					}
					HexGroupToHexString(SerialUnion.S_DatasStruct.Datas,Buffer,8);
					printf_debug("下载传输密钥== %s\r\n",Buffer);
					sqlite_update_commEncryptKey_config_db(Buffer);
					bitUpdateParameter=1;	
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;	
			break;
		case RD_ENCRYPTKEY_COMD://上传传输密钥
			memcpy(SerialUnion.S_DatasStruct.Datas,commEncryptKey,8);
			SerialUnion.S_DatasStruct.DatasLen=8;
			break;
		case RD_CONSUMMONEY_COMD://读出消费额
			/*if (!SerialUnion.S_DatasStruct.UartAddrL)//A值
				SumMoney=Sys_SumConsumMoney;
			else if (SerialUnion.S_DatasStruct.UartAddrL==1)
			{//L值
				SumMoney=0;
				ii=NoCollectRecordIndex;
				i=0;
				while (ii!=SaveRecordIndex)
				{
					Addr=(ulong)ii*RECORD_SIZE;
				//	PPage=*(1+(uchar *)&iii);
				//	memcpy((uchar *)&Addr,(2+(uchar *)&iii),2);
					Flash_Rd_Bytes(Addr,DatasBuffer,32);
					if (DatasBuffer[0]==0xa0 && !BytesCheckSum(DatasBuffer+1,31)&& !(DatasBuffer[1]&0x80))
						SumMoney+=ChgBCDStringToUlong(DatasBuffer+15,3);
					ii=(ii+1)%MAXRECORD_NUM;
					i++;
					if (!(i%64))
						SerialSendChar(STX);
				}
			}
			else if (SerialUnion.S_DatasStruct.UartAddrL==2 && SerialUnion.S_DatasStruct.DatasLen==2)
			{//D值
				InitSaveRecordSub(0);
				SumMoney=0;
				ii=ReCollectRecordIndex;
				i=0;
				while (ii!=SaveRecordIndex)
				{
					Addr=(ulong)ii*RECORD_SIZE;
					//PPage=*(1+(uchar *)&iii);
					//memcpy((uchar *)&Addr,(2+(uchar *)&iii),2);
					Flash_Rd_Bytes(Addr,DatasBuffer,32);
					if (!DatasBuffer[0] || DatasBuffer[0]==0xa0)
					{
						if (!BytesCheckSum(DatasBuffer+1,31)&& !(DatasBuffer[1]&0x80))
						{
							ChgRecordDatasToTime(DatasBuffer+22,aaa);
							if ((!SerialUnion.S_DatasStruct.Datas[0]|| SerialUnion.S_DatasStruct.Datas[0]==aaa[1]) && 
								(!SerialUnion.S_DatasStruct.Datas[1]|| SerialUnion.S_DatasStruct.Datas[1]==aaa[2])	)
								SumMoney+=ChgBCDStringToUlong(DatasBuffer+15,3);
						}
					}
					i++;
					if (!(i%64))
						SerialSendChar(STX);
				}
			}
			else
				SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			SumMoney = FourBigToSmall(SumMoney);
			memcpy(SerialUnion.S_DatasStruct.Datas,(uchar *)&SumMoney,4);
			SumMoney = FourBigToSmall(SumMoney);
			SerialUnion.S_DatasStruct.DatasLen=4;
			break;
			*/
		case	CLR_NENU_COMD://清菜单
			/*SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(CLR_NENU_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					Addr=MenuPrince_Addr;
					for (ii=0;ii<280;ii++)
					{
						if (!(ii%70))
						{
							SerialSendChar(STX); 
//							TCP_Send_STX();
						}						
						j=0xff;
						Clr_WatchDog();
						WrBytesToAT24C64(Addr++,&j,1);
						Clr_WatchDog();
					}
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			*/
			break;
		case	SET_MENU_COMD://设置菜单
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
			/*if (SerialUnion.S_DatasStruct.UartAddrL<70 && !(SerialUnion.S_DatasStruct.DatasLen%3))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;	
				if ((SerialUnion.S_DatasStruct.UartAddrL+(SerialUnion.S_DatasStruct.DatasLen/3) )<71)
					PPage=SerialUnion.S_DatasStruct.DatasLen/3;
				else
					PPage=70-SerialUnion.S_DatasStruct.UartAddrL;	
				Addr=MenuPrince_Addr+(uint)SerialUnion.S_DatasStruct.UartAddrL*4;
				for (i=0;i<PPage;i++)
				{
					memcpy(DatasBuffer,SerialUnion.S_DatasStruct.Datas+i*3,3);
					DatasBuffer[3]=CalCheckSum(DatasBuffer,3);
					WrBytesToAT24C64(Addr,DatasBuffer,4);
					memset(DatasBuffer,0,4);
					RdBytesFromAT24C64(Addr,DatasBuffer,4);
					Addr+=4;
					Clr_WatchDog();
				}
			}*/
			
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	RD_MENU_COMD://读取菜单
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
			/*if (SerialUnion.S_DatasStruct.UartAddrL<70)
			{
				SerialUnion.S_DatasStruct.UartStatus=0;	
				Addr=MenuPrince_Addr+(uint)SerialUnion.S_DatasStruct.UartAddrL*4;
				if ((71-SerialUnion.S_DatasStruct.UartAddrL)>32)
					status=32;
				else
					status=70-SerialUnion.S_DatasStruct.UartAddrL;
				PPage=0;
				for (i=0;i<status;i++)
				{
					Clr_WatchDog();
					RdBytesFromAT24C64((Addr+i*4),DatasBuffer,4);
					if (!BytesCheckSum(DatasBuffer,4) && !BCD_String_Diag(DatasBuffer,3))
					{
						memcpy(SerialUnion.S_DatasStruct.Datas,DatasBuffer,3);
						PPage+=3;	
					}
					else
						break;
				}
				if (PPage)
					SerialUnion.S_DatasStruct.DatasLen=PPage;
				else
					SerialUnion.S_DatasStruct.UartStatus=NoDatas_Error;
			}
			*/
			break;
		case	CLR_MENUNAME_COMD://清除菜单
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			/*if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(CLR_MENUNAME_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					Erase_One_Sector(MenuName_Addr);
					Clr_WatchDog();
				}
			}*/
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	SET_MENUNAME_COMD://设置菜名
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;	
			/*if (SerialUnion.S_DatasStruct.UartAddrL<70 && !(SerialUnion.S_DatasStruct.DatasLen%16))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;	
				if ((SerialUnion.S_DatasStruct.UartAddrL+(SerialUnion.S_DatasStruct.DatasLen/16) )<71)
					PPage=SerialUnion.S_DatasStruct.DatasLen/16;
				else
					PPage=70-SerialUnion.S_DatasStruct.UartAddrL;	
				Addr=MenuName_Addr+(uint)SerialUnion.S_DatasStruct.UartAddrL*16;
				for (i=0;i<PPage;i++)
				{
					memcpy(DatasBuffer,SerialUnion.S_DatasStruct.Datas+i*16,16);
					Flash_Write_Bytes(Addr,DatasBuffer,16);
					Addr+=16;
					Clr_WatchDog();
				}
			}
			*/
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	CLR_SORTPRINCE_COMD://清除价格方案
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(CLR_SORTPRINCE_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					sqlite3_moneyplan_clr_db();
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	SET_SORTPRINCE_COMD://设置价格方案
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen && !(SerialUnion.S_DatasStruct.DatasLen%13))
			{
				SerialUnion.S_DatasStruct.UartStatus=0;	
				PPage=SerialUnion.S_DatasStruct.DatasLen/13;
				ii=SerialUnion.S_DatasStruct.UartAddrL+PPage;
				if (ii>200)
					PPage=(uint)200-SerialUnion.S_DatasStruct.UartAddrL;
				for (i=0;i<PPage;i++)
				{
					// DatasBuffer[0]=0xa0;
					// memcpy(DatasBuffer+1,SerialUnion.S_DatasStruct.Datas+i*13,13);
					// DatasBuffer[14]=CalCheckSum(DatasBuffer,14);
					// stru.serid = MoneyPlanIndex;
					// gdHexGroupToHexString(DatasBuffer,Buffer,15);
					// memcpy(stru.recoedDatas,Buffer,30);
					// sqlite3_moneyplan_insert_db(stru);
					// MoneyPlanIndex++;
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	RD_SORTPRINCE_COMD://读出价格方案
			/*Addr=SortPrince_Addr+SerialUnion.S_DatasStruct.UartAddrL*16;
			if (((uint)256-SerialUnion.S_DatasStruct.UartAddrL)<8)
				PPage=(uint)256-SerialUnion.S_DatasStruct.UartAddrL;
			else
				PPage=8;
			SerialUnion.S_DatasStruct.DatasLen=0;
			for (i=0;i<PPage;i++)
			{
				Flash_Rd_Bytes(Addr,DatasBuffer,15);
				if (DatasBuffer[0]==0xa0 && !BytesCheckSum(DatasBuffer,15))
				{
					memcpy(SerialUnion.S_DatasStruct.Datas+i*13,DatasBuffer+1,13);
					SerialUnion.S_DatasStruct.DatasLen+=13;
				}
				else
					break;
			}
			break;*/

		case	RD_RECORD_COMD://采集交易记录
			ii=NoCollectRecordIndex;
			SerialSendNoCollectNum=0;
			printf_debug("NoCollectRecordIndex = %d\n",NoCollectRecordIndex);
			printf_debug("SaveRecordIndex = %d\n",SaveRecordIndex);
			if (SerialUnion.S_DatasStruct.DatasLen==1)
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
				Nums=SerialUnion.S_DatasStruct.Datas[0];
				if (Nums>3)
					Nums=3;
				i=0;
				j=10;
				SerialUnion.S_DatasStruct.DatasLen=0;
				while( ( ii!=SaveRecordIndex ||  bitRecordFull ) && i<Nums && j)
				{
					sqlite3_consume_query_record_db(NoCollectRecordIndex +SerialSendNoCollectNum);//数据库中查找记录
					printf_debug("recordSt.recordId    = %d\n",recordSt.recordId);
					printf_debug("recordSt.recoedDatas = %s\n",recordSt.recoedDatas);
					HexStringToHexGroup(recordSt.recoedDatas, DatasBuffer, 32);
					SerialSendNoCollectNum++;
								
					if (DatasBuffer[0]==0xa0 && !BytesCheckSum(DatasBuffer+1,31))
					{
						memcpy(SerialUnion.S_DatasStruct.Datas+SerialUnion.S_DatasStruct.DatasLen,DatasBuffer+1,31);
						SerialUnion.S_DatasStruct.DatasLen+=31;
						i++;
					}
					else
					{
						printf_debug("checksum fail \n");
					}
					
					ii=(ii+1)%MAXRECORD_NUM;
					j--;
				}
				if (!SerialUnion.S_DatasStruct.DatasLen)
				{
					if (ii==SaveRecordIndex)
						SerialUnion.S_DatasStruct.UartStatus=NoDatas_Error;//记录采集完成
					else
						SerialUnion.S_DatasStruct.UartStatus=Running_Status;//正在查找数据		
				}
			}
			break;
		case	DEL_RECORD_COMD://交易记录移指针
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(DEL_RECORD_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					if (SerialSendNoCollectNum)
					{
						while (( NoCollectRecordIndex!=SaveRecordIndex || bitRecordFull) && SerialSendNoCollectNum )
						{
							sqlite3_consume_move_db(NoCollectRecordIndex,SerialSendNoCollectNum);//已采记录数据库操作
		
							NoCollectRecordIndex = (NoCollectRecordIndex+SerialSendNoCollectNum)%MAXRECORD_NUM;
							SerialSendNoCollectNum = 0;
						}
					}
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	INIT_RECORD_PTR_COMD://初始化交易记录指针
//			sqlite3_consume_query_collectedRecord_db(aaa);
//			ReCollectRecordIndex = recordSt.recordId;
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	RD_RERECORD_COMD://复采交易记录
			SerialSendReCollectNum=0;
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==3)
			{
				SerialUnion.S_DatasStruct.UartStatus=0;
				Nums=SerialUnion.S_DatasStruct.Datas[0];//采集的个数
				if (Nums>3)
					Nums=3;
				Yearchar = SysTimeDatas.S_Time.YearChar;//采集的年
				MonthChar=SerialUnion.S_DatasStruct.Datas[1];//采集的月
				DayChar=SerialUnion.S_DatasStruct.Datas[2];//采集的日
				aaa[0] = Yearchar;
				aaa[1] = MonthChar;
				aaa[2] = DayChar;
				iii = ChgBCDStringTouint32_t(aaa, 3);
				printf_debug("澶嶉噰鏃舵湡= %d\n",iii );
				if(MonthChar !=SerialUnion.S_DatasStruct.Datas[1] ||DayChar !=SerialUnion.S_DatasStruct.Datas[2])
				{
					sqlite3_consume_query_collectedRecord_db(iii);
					ReCollectRecordIndex = recordSt.recordId;
					ii=ReCollectRecordIndex;
					printf_debug("ReCollectRecordIndex =%d\n",ReCollectRecordIndex);
				}
				i=0;
				SerialUnion.S_DatasStruct.DatasLen=0;
				bitHaveReCollectRecord=0;
				j=10;
				while ((ii!=SaveRecordIndex ||bitRecordFull) && i<Nums && j)
				{	
					sqlite3_consume_query_record_db(ReCollectRecordIndex +SerialSendNoCollectNum);//数据库中查找记录
					printf_debug("recordSt.recordId    = %d\n",recordSt.recordId);
					printf_debug("recordSt.recoedDatas = %s\n",recordSt.recoedDatas);
					HexStringToHexGroup(recordSt.recoedDatas, DatasBuffer, 32);
					SerialSendNoCollectNum++;
					printf_debug("recordSt.ConsumeTime= %d\n",recordSt.ConsumeTime);
								
					if (DatasBuffer[0]==0xa0 && !BytesCheckSum(DatasBuffer+1,31) && iii==recordSt.ConsumeTime)
					{
						memcpy(SerialUnion.S_DatasStruct.Datas+SerialUnion.S_DatasStruct.DatasLen,DatasBuffer+1,31);
						SerialUnion.S_DatasStruct.DatasLen+=31;
						i++;
					}
					else
					{
						printf_debug("checksum fail \n");
					}
				
					ii=(ii+1)%MAXRECORD_NUM;
					//SerialSendReCollectNum++;
					bitHaveReCollectRecord=1;
					j--;
				}
				if (!SerialUnion.S_DatasStruct.DatasLen)
				{
					if (ii==SaveRecordIndex)
						SerialUnion.S_DatasStruct.UartStatus=NoDatas_Error;//记录采集完成
					else
						SerialUnion.S_DatasStruct.UartStatus=Running_Status;//正在查找数据	
				}
			}
			break;
		case	DEL_RERECORD_COMD://复采记录移指针
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(DEL_RERECORD_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
					SerialUnion.S_DatasStruct.UartStatus=0;
					if (bitHaveReCollectRecord)
					{
						bitHaveReCollectRecord=0;
						while (( ReCollectRecordIndex!=SaveRecordIndex || bitRecordFull)  &&  SerialSendReCollectNum )
						{
							ReCollectRecordIndex=(ReCollectRecordIndex+1)%MAXRECORD_NUM;
							SerialSendReCollectNum--;
					 	}
					}	
				}
			}
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
		case	LOAD_PROGAM_COMD://固件更新
			SerialUnion.S_DatasStruct.UartStatus=ReDatas_Error;
			/*if (SerialUnion.S_DatasStruct.DatasLen==6)
			{
				CalComdSymblDatas(LOAD_PROGAM_COMD,DatasBuffer);
				if (!memcmp(SerialUnion.S_DatasStruct.Datas,DatasBuffer,6))
				{
				
					bitNeedDownLoad=1;
					SerialUnion.S_DatasStruct.UartStatus=0;
				}
			}
		
			SerialUnion.S_DatasStruct.DatasLen=0;*/
			break;
		default:
			SerialUnion.S_DatasStruct.UartStatus=ReComd_Error;
			SerialUnion.S_DatasStruct.DatasLen=0;
			break;
	}
	udpSendSub();
}

static void	UdpAscToHex(uint8_t * Sum,uint8_t aa)
{
	uint8_t	bb;
	bb=aa>>4;
	if (bb<10)
		bb+=0x30;
	else
		bb+=0x37;
	Sum[0]+=bb;
	Tx_Buffer[udp_len++] = bb;

	bb=aa&15;
	if (bb<10)
		bb+=0x30;
	else
		bb+=0x37;
	Sum[0]+=bb;
	
	Tx_Buffer[udp_len++] = bb;
		
}

/*==================================================================================
* 函 数 名： udp_instance_init
* 参    数： None
* 功能描述:  udp初始化
* 返 回 值： None
* 备    注： None
* 作    者： xiaozh
* 创建时间： 2019-10-28 170617
=================================================================================*/
void udp_instance_init(void)
{
	msg_queue_init(&udp_queue);
}

/*==================================================================================
* 函 数 名： udp_push_data_msg
* 参    数： None
* 功能描述:  udp 接收到数据后调用此函数把接收的数据添加的队列里面
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2021-09-29 170658
==================================================================================*/
void udp_push_data_msg(void* ret_msg)
{
	_pUdp_Msg pmsg = ret_msg;  
	
	msg_queue_push(&udp_queue, pmsg);
}

static void udpSendSub(void)//
{
	uint8_t	i;
	uint8_t buf[200];
	uint16_t	Sum;
	uchar	CheckSum;
	_Udp_Msg pmsg ;

	if (!bitSerial_SendRequest)	
		return;

	bitSerial_SendRequest=0;
	udp_len = 0;
	Tx_Buffer[udp_len++] = STX;
	SerialUnion.S_DatasStruct.UartSeAddrCode= pdevMsg.maincode ;
	SerialUnion.S_DatasStruct.UartReAddrCode=0;
	if (SerialUnion.S_DatasStruct.UartStatus)
		SerialUnion.S_DatasStruct.DatasLen=0;

	SerialUnion.S_DatasStruct.UartReAddrCode = DoubleBigToSmall(SerialUnion.S_DatasStruct.UartReAddrCode);
	SerialUnion.S_DatasStruct.UartComd = DoubleBigToSmall(SerialUnion.S_DatasStruct.UartComd);
	SerialUnion.S_DatasStruct.UartSeAddrCode = DoubleBigToSmall(SerialUnion.S_DatasStruct.UartSeAddrCode);
	
	Sum=Cal_CRC_Sub(SerialUnion.S_DatasBuffer,SerialUnion.S_DatasStruct.DatasLen+11);

	for(i=0;i<SerialUnion.S_DatasStruct.DatasLen+11;i++)
	{
		UdpAscToHex(&CheckSum,SerialUnion.S_DatasBuffer[i]);	
	}
	Tx_Buffer[udp_len++] = ETX;
	UdpAscToHex(&CheckSum, Sum/256);
	UdpAscToHex(&CheckSum, Sum%256);

	Init_Serial();
	printf_debug("发送数据==");
	for(i=0;i<udp_len;i++)
		printf_debug("%2X ",Tx_Buffer[i]);
	printf_debug("\r\n");
	HexGroupToHexString(Tx_Buffer,buf,udp_len);
	pmsg.byte_count = udp_len*2;
	memcpy(pmsg.data,buf,udp_len*2);
	udp_push_data_msg(&pmsg);//接收的数据添加到队列里面
}

/*==================================================================================
* 函 数 名： udp_response_parse
* 参    数： None
* 功能描述:  udp socket 接收到的数据解析
* 返 回 值： None
* 备    注： 
* 作    者： lc
* 创建时间： 2021-06-20 025540
==================================================================================*/
void udp_response_parse(void* ret_msg)
{
	int i=0;
	_pRet_Msg pmsg = ret_msg;

	printf_debug("pmsg->data == ");
	for(i=0;i<pmsg->byte_count;i++)
		printf_debug("%2x ", pmsg->data[i]);
	printf_debug("\n");
	udp_send_data(pmsg->data);
}
/*==================================================================================
* 函 数 名： udp_pop_one_frame
* 参    数： _pRet_Msg
* 功能描述:  udp数据接收
* 返 回 值： None
* 备    注： None
* 作    者： lc
* 创建时间： 2021-06-20 154449
==================================================================================*/ 
uint8_t udp_pop_one_frame(void )
{
	int num,i;
	p_send_queue_t p_queue_buff = &udp_queue;
	
	//检查队列里面是否有数据
	num = msg_queue_num(p_queue_buff);
	if(num)
	{
		for(i=0; i<MAX_CACHE_NUM; i++)
		{
			if(p_queue_buff->queue[i].byte_count)
			{
				printf_debug("队列里面的消息 =%s \n", p_queue_buff->queue[i].data);
				//debug_print_hex(p_queue_buff->queue[i].data,10);
				udp_response_parse(&(p_queue_buff->queue[i]));
				msg_queue_pop(p_queue_buff, 0);
				//return 0;
			}	
		}
		//printf_debug("队列里面的消息个数 =%d \n",num);
	}
}
