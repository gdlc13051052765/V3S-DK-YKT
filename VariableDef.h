
#include <string.h>
#include <stdio.h>
#include "mydefine.h"
#include <unistd.h>
#include "sysTime.h"

uint16_t	MainCode ;//站点号
uint16_t	UdpCode = 9000;//UDP端口号
uint8_t		OutChar16C550;
uint8_t		CommModeChar=0;

uint8_t		bitPinEnable=0;//超限额密码消费使能
uint8_t		bitNeedRST=0;
uint8_t		bitNeedDownLoad=0;
uint8_t		bitPurseEnd=0;

uint16_t	SerialReceiveTimeCount=0;
uint8_t		TCPDatasComd;
uint8_t		LedTcpIndex;//显示TCPIP网络参数的索引

uint8_t		DispCount;
uint8_t		PinCount=0;
uint8_t		bitHaveReadBalance=0;
uint8_t		bitSetDateEnable=0;//时间设置使能 
uint8_t		bitWrittingFlash=0;
uint8_t		bitDispFlash=0;
uint8_t		bitBeepError=0;
uint8_t		bitHaveCollectRecord=0;
uint8_t	  	bitHaveReCollectRecord=0;
uint8_t		bitStatus8563=0;
uint8_t		bitSysTimeDisable=0;
uint8_t		bitRecordFull=0;
uint8_t		bitConsumZeroEnable=0;//限次
uint8_t		bitHaveSeclectPurse=0;
uint8_t		bitCommStatus;//读卡状态备份

uint8_t		StatusBak;//是否有卡
uint8_t		DelayReadCardTimeCount=0;
uint8_t		CtrlChar=0xff;//控制字节输出，主要通过573控制蜂鸣器和485通讯的收发转换
uint8_t		LoopCount=0;//主循环计数
uint8_t		bitUpdateParameter=1;
uint8_t		DownLoadPragamSymbl[6];//需要下载程序的标识数据,0xb0--0xb5,需要下载
uint8_t		LoadProgamReDatas[6] ;
uint8_t		ConsumCase=0;
uint8_t		ConsumMode;//消费方式

uint8_t		T3_TimeCount=0;
uint8_t		bitHaveLedError=0;
uint8_t		bitNeedDiagRecordFull=1;

uint8_t		InputCase=0;
uint8_t		InputBuffer[8];
uint8_t		InputCount=0;
uint8_t		bitHaveInputDatas=0;
uint8_t		InputNum=0;
uint8_t		bitInPutPoint=0;
uint8_t		InputMaxNum=0;
uint16_t	MulData=0;
uint32_t	SingleMoney=0;

uint32_t	Sys_SumConsumMoney=0;	

uint8_t		PurseUsingNum;//当前钱包号
uint8_t		SelectPurseNum;//应用钱包起始号
/////////////////////系统参数///////////////
uint8_t		CardMinBalance[3];//卡底金
uint8_t		DayLimetMoney[3];//下载的日限额
uint8_t		CardEnableMonths;//黑名单有效月数

uint8_t		ConsumModeEnable;//消费方式允许
uint32_t	SumConsumMoney=0;//总消费额
uint32_t	SumPrintMoney=0;//打印的总消费额
uint8_t		bitNeedPrinter=0;//1-需要打印
uint32_t	CurrentConsumMoney=0;//本次消费额
uint8_t		MenuSort=0;//菜号
uint8_t		InputMenuSort=0;
uint8_t     LoadModeResponse[15];
uint8_t     LoadModeFlag = 0;//ad by lc

uint16_t	BeepDelayTimeCount=0;//蜂鸣器
uint16_t    BeepTimes=0;
uint8_t		bitHaveKey=0;
uint8_t		bitHaveReleaseKey=1;
uint32_t	KeyValue;
uint8_t		Receive_Ip_Port[6];

union		sDatasUnion  SerialUnion;
uint8_t		SerialReceiveLen;
uint8_t		SerialReCase;
uint8_t		rChar;
uint16_t	SerialReCheckSum;
uint8_t		SerialReCheckSumCount;

uint8_t     bitUARTSendEnd;
uint8_t		bitSerial_ReceiveEnd;
uint8_t		bitSerial_SendRequest;
uint8_t		bitEven;

uint8_t		bitPF8563Error=0;
union		uTimeUnion	SysTimeDatas;
union		uTimeUnion	SysTimeDatasBak;

uint32_t	RecordSum=0;//记录总数
uint32_t	NoCollectRecordSum=0;//没有采集的记录总数
uint32_t	NoCollectRecordIndex=0;//没有采集的记录指针
uint32_t	ReCollectRecordIndex=0;//已经采集的记录指针
uint32_t	SaveRecordIndex=0;//存储记录指针
uint8_t		SerialSendNoCollectNum=0;//上次传送的未采记录个数
uint8_t		SerialSendReCollectNum=0;//上次传送的复采记录个数
uint8_t		MoneyPlanIndex = 0;//价格方案数据库存储指针

uint8_t		PaultRate9600Symbl[4]={0x46,0x57,0x68,0x79};
uint8_t		PaultRate4800Symbl[4]={0x0a,0x1b,0x2c,0x3d};
uint8_t		DownLoadDatas[6]={0xb0,0xb1,0xb2,0xb3,0xb4,0xb5};

//---------------------------------------------------------------
uint8_t     ConsumSndInfo[60];//CPU卡发送信息
uint8_t     CPU_CommBuf[50];
uint8_t     *CPU_RcLen;
uint8_t     PsamRcvBufCount;
uint8_t     PsamRcvBuf[50];//psam

//卡UID
uint8_t		CardType[2];
uint8_t		CardSerialNum[4];
uint8_t		CardSerialNumBak[4];
uint8_t		CardPrinterNum[4];//印刷编号

//一卡通公用信息
//uint8_t		MatchCode[4]={0x12,0x34,0x56,0x78}; //UserCode[4];//用户代码
uint8_t		CardIdentity;//身份
uint8_t		CardBatch;//卡批次
uint32_t	Limit_DayMoney;//日限额
uint32_t	Max_ConsumMoney;//单笔限额
//uint8_t   	Card_Rebate;//折扣
uint8_t		PursesSector[10];//钱包文件标识索引
uint8_t     nameBuf[16];//名字

//一卡通累计信息
uint32_t	CardDayConsumMoney;//卡上的日消费额
uint8_t		CardConsumDate[3];//卡上的日消费日期,时间
uint8_t		ConsumCountDateTime[4];//计次消费日期时间
uint8_t		CardDayConsumCount=0;//累计次数    //CardPurseDayCount

//一卡通交易信息
uint8_t		PinCheckSta; //PIN校验状态
uint8_t   	Sys_PinCode[3]={0x12,0x34,0x56}; //系统PIN码
uint8_t		PurseContrlNum[2];//卡交易号
uint16_t	PosConsumCount=0;//POS的消费流水号
uint8_t		CardConsumTime[6];//消费时间，防止MAC校验失败
uint8_t		Mac2Bak[4];//MAC2
uint8_t		WriteCardSta=0;//写卡错误状态
uint8_t		OldBalance[4];
uint8_t		NewBalance[4];

uint8_t     ComChallenge[5]={0x00,0x84,0x00,0x00,0x04};//取随机数

uint8_t 	keybuff[4];
uint32_t    Falsh_TimeCount=0;
uint8_t     Forbidden=0;
uint8_t     ReadKey_Pause=0; 
uint8_t	    DispBuffer[11];//显示数据
uint8_t	    LedWattingChar=0;
uint8_t	    bitAddStatus=0;

uint8_t	    bitHaveAuto=0;
uint8_t	    bitLookSysDatas=0;
uint8_t	    bitLookD=0;
uint8_t	    bitUseMoneyBlock;

uint8_t 	NetCardDatas[22];
uint8_t 	TypeA_Sort;//M1=0/CPU=1
uint8_t 	Bak_Sort;

//uint8_t	    CardKeyCode[6];//读卡密码
//uint8_t	    CalCardKey[8];//卡密钥
//uint8_t	    CardSector;//公共区的扇区号 
uint8_t	    PinCode[3];
uint8_t	    Limit_MoneySign = 0xff;
uint8_t	    DaySumMoneyDatasBak[28];//欲覆盖的日累计消费额的数据备份

uint8_t	    PurseWrBufferBak[25];
uint8_t	    MainWrBufferBak[29];

uint8_t	    PurseConsumDateTime[4];
uint32_t	PurseSumConsumMoney;
uint8_t	    PurseBT_Num[2];

uint8_t  	DefaultKind[4]={0xab,0xcd,0xef,0xfe};
uint8_t	    Flag_NotDefault;
uint8_t     Flag_BakRecode;
uint8_t	    PurseDatas_Info[24];//钱包数据备份

uint8_t  	rebufff[70];//串口接收到的数据 
uint8_t  	ser0_BytesReceived=0;//已接收到的数据个数
uint8_t  	ser0_ReceiveLen=0;//期望接收到的个数
 
uint16_t   	RecDelay = 0;   //接受数据延时等待	
//ad by lc
uint8_t 	Udp_Send_Flag = 0;
uint32_t 	Led_Open_time = 0;
uint8_t 	STX_FLAG = 0;
uint8_t 	wifi_mode_flag = 0;
uint8_t 	BatModeFlag;
uint8_t  	CpuID[3];
uint16_t 	POSServerPort = 9000;//客户端端口号

uint8_t	    ReWriteCardDatasBak[16];//写卡前将要覆盖的数据
uint16_t 	ClientPort = 9001;//客户端端口号
uint8_t 	RemoteIP[4]={192,168,10,188};
uint8_t 	RemoteIPFlag = 1;

uint8_t 	EscComsumeSaveRecordFlag = 0;
uint8_t 	BeepFlag = 0;
uint8_t	    NeedResartFlag = 0;
uint32_t    LocalTime = 0;
uint8_t     FirstSendStx = 0;
uint8_t     Rebate_YesOrNo_Flag = 0;
uint8_t     DayLimetFlag = 0;//0时日限额按卡里面的 1按下载的
uint32_t    ResetEnc28j60Num = 0;
uint8_t     FirstUdpConnect=0;//第一次建立连接

uint8_t 	VoiceVal = 30;
uint8_t     Reset_Wifi_Flag = 0;
uint8_t     ShuRuMoney_Err = 0;//金额输入不完全
