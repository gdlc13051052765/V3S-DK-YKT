#include "ExternVariableDef.h"
#include "sysTime.h"
#include "debug_print.h"
#include "sqliteTask.h"
#include "mydefine.h"

uchar	RequestCard(void);
uchar	Detect_Card(void);
uchar   CheckCardPrinterNum(void);//拔卡重新插入后的校验
uchar   SearchPurseBalnanceDatas(uchar,uchar, uchar *,uchar *);//查找钱包
uchar	CurrentConsumMoneyDiag(uchar Sort );//卡种类和消费额诊断
uchar   ReadCardCommonDatas(void);
uchar	ReadCardBalanceDatas(uchar PurseNum);//读余额
uchar	ReadCard_DaySumConsumMoney(void);//读出累计日消费额
uchar   ConsumPocess(void);
uchar 	WriteCardLjdata(void);

extern uint8_t PurseEnable[10];
//测试是否有卡
uchar RequestCard(void)
{
	return check_RFCPU_ResetInfo(CPU_RcLen,CPU_CommBuf);//卡复位信息
}

//侦测卡片是否在依然存在
uchar Detect_Card(void)
{
	uchar status;	

	status=check_RFCPU_ResetInfo(CPU_RcLen, CPU_CommBuf);//卡复位信息
	if(status)
		status=CARD_NOCARD;
	else
	{
		if(TypeA_Sort)
			status=CARD_SAME_ERROR;		
		status=memcmp(CardSerialNumBak, CardSerialNum, 4);//唯一序列号
		if(status)
			status=CARD_SAME_ERROR;	
	}
		
	return	status;	
}

//拔卡重新插入后的校验
uchar CheckCardPrinterNum(void)
{
	uchar status;

    status=RequestCard();
    if(status)
    	return  status;
    if(Bak_Sort!=TypeA_Sort)
    	return 	CARD_SAME_ERROR;
	
	status=memcmp(CardSerialNumBak, CardSerialNum, 4);//唯一序列号
	if(status)
		return	CARD_SAME_ERROR;
	
	if(!TypeA_Sort)
		return 0;
		
	return	0;
}

//读卡的公共信息
uchar ReadCardCommonDatas(void)
{
	uchar	status;
	
	Bak_Sort=TypeA_Sort;
	status=ReadCardCommonDatas_M1();
	if(status)
		return	status;	
	status=ReadCard_DaySumConsumMoney();	
	return	status;
}
//读出累计日消费额
uchar ReadCard_DaySumConsumMoney(void)
{
	uchar status;	

	status=ReadCard_DaySumConsumMoney_M1();
	return	status;		
}
//读余额
uchar ReadCardBalanceDatas(uchar PurseNum)
{
	uchar status;	

	status=ReadCardBalanceDatas_M1(PurseNum);
	return	status;		
}

//-------------------------------------------------------------------
//PurseEnable--钱包的使能顺序
//从aa开始找，找到写入PurseNum，PurseNumEnd为结束位置
//PursesSector为允许消费钱包索引
//-------------------------------------------------------------------
uchar SearchPurseBalnanceDatas(uchar Sort, uchar aa, uchar * PurseNum , uchar * PurseNumEnd)//查找钱包
{
	uchar	i,j;
	uchar	status;
	uchar	Buffer[16];
	ulong	addr;

	if(!Flag_NotDefault)
		memcpy(PurseEnable,"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09",10);//钱包是否有效

	status=NO_PURSE_ERROR;
	// memcpy(PurseEnable+3,"\x03",1);//钱包是否有效
	bitPurseEnd=0;
	for (i=aa;i<10;i++)
	{
		if (PurseEnable[i]<10)//钱包索引合法
		{
			j=PurseEnable[i];//钱包号
			if (PursesSector[j]!=0xff)//卡允许此钱包消费
			{					
				 status=ReadCardBalanceDatas(PursesSector[j]);
				 if(!status)
				 {
				    PurseNum[0]=j;		
					if (i==9)//最后一个钱包
						bitPurseEnd=1;
					else
					{
						j=PurseEnable[i+1];//下一个钱包号
						if (j>9 || PursesSector[j]==0xff)
							bitPurseEnd=1;
					}
					break;
			 	}
			}
		}
	}
	if (i<9)
		PurseNumEnd[0]=i+1;
	else
		PurseNumEnd[0]=0;
	return	status; 					
}

//消费过程控制，包含写卡和写累计
uchar ConsumPocess(void)
{
	uchar status;
	
	PurseUsingNum = 0;
	status=WriteBalanceToCard_M1(0,PursesSector[PurseUsingNum]);	

	return status;
}

//卡种类和消费额诊断
//传入参数，卡类型 ,CardConsumDate-上次的消费时间
//返回0-有效的价格，最新消费次数－CardConsumCount，单价－CurrentConsumMoney;
uchar CurrentConsumMoneyDiag(uchar Sort )
{
	uchar	i;
	uint8_t	Buffer[30],Buf[30];
	ulong	Addr;
	uchar	bitHaveFind=0;
	uchar	status;
	struct sMoneyplanStruct stu;
	
	bitConsumZeroEnable=0;
	
	for(i=0;i<200;i++)
	{
		stu= sqlite3_moneyplan_query_db(i);
		printf_debug("价格方案== %s\n",Buf);
		HexStringToHexGroup(stu.recoedDatas , Buffer, 30);
		printf_debug("价格方案==");
		for(i=0;i<15;i++)
		{
			printf_debug("%2x ",Buffer[i]);
		}
		printf_debug("\n");
		
 		if (Buffer[0]!=0xa0)
			return	CARD_LIMIT_NUM;
		if (Buffer[1]==Sort && !BytesCheckSum(Buffer,15))
		{
			status=memcmp(SysTimeDatas.TimeString+3,Buffer+2,2);
			if (status!=0xff)
			{
				//status=memcmp(SysTimeDatas.TimeString+3,Buffer+4,2);
				if(SysTimeDatas.TimeString[3]<Buffer[4])
				{
					bitHaveFind=1;//找到符合身份和时间段的价格方案
					break;
				}
				else
				{
					if(SysTimeDatas.TimeString[3]==Buffer[4])
					{
						if(SysTimeDatas.TimeString[4]<=Buffer[5])
						{
							bitHaveFind=1;//找到符合身份和时间段的价格方案
							break;	
						}	
					} 
				} 
			}
		}
	}
	if(!bitHaveFind)
		return	CARD_LIMIT_NUM;
	status=1;
	if(memcmp(ConsumCountDateTime, SysTimeDatas.TimeString+1, 2))
		status=0;//上次消费时间和当前时间不在同一天
	else {
		status=memcmp(ConsumCountDateTime+2,Buffer+2,2);//上次消费时间在当前时间之前
		if(ConsumCountDateTime[2]<Buffer[2]) {
			status=0;	
		} else {
			if(ConsumCountDateTime[2]==Buffer[2]) {
				if(ConsumCountDateTime[3]<Buffer[3])
					status=0;
				else
					status=1;
			} else {
				status=1;
			}
		} 
	}
	if(!status) {//上次消费不在本时间段内
		CurrentConsumMoney=ChgBCDStringToUlong(Buffer+6,3);
		memcpy(ConsumCountDateTime, SysTimeDatas.TimeString+1, 4);//更新计次时间
		CardDayConsumCount=1;
		if (!CurrentConsumMoney)
			bitConsumZeroEnable=1;
		return	0;
	} else {//上次消费时间在本次消费时间内
		if (CardDayConsumCount<Buffer[9]) {
			CurrentConsumMoney=ChgBCDStringToUlong(Buffer+6,3);
			memcpy(ConsumCountDateTime,SysTimeDatas.TimeString+1,4);//更新计次时间
			CardDayConsumCount++;
			if (!CurrentConsumMoney)
				bitConsumZeroEnable=1;
			return	0;
		} else {
			i=CardDayConsumCount-Buffer[9];
			if (i<Buffer[13]) {
				CurrentConsumMoney=ChgBCDStringToUlong(Buffer+10,3);
				memcpy(ConsumCountDateTime,SysTimeDatas.TimeString+1,4);//更新计次时间
				CardDayConsumCount++;
				if (!CurrentConsumMoney)
					bitConsumZeroEnable=1;
				return	0;
			}
			else
				return	CARD_LIMIT_NUM;
		}
	}
}

uchar WriteCardLjdata(void)
{
	uchar status;	

	status=ReWriteCardSub_M1(1);
	return	status;	
}
