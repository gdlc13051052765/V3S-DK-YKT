package main

/*
#include <stdio.h>
#include "sqliteTask.h"
#include "msgTask.h"
#include "readWriteCard.h"
#include "rewrCardTask.h"
#include "udp_data_analyse.h"
#include "cAppTask.h"
#cgo CFLAGS: -I./
#cgo LDFLAGS: -L./ -lsqlite3  -lm
*/
import "C"

//设备信息
type DeviceMsgSt struct {
	Version          string //版本号
	Maincode         int    //设备号
	LocalIpaddr      string //ip地址
	RemoteIpaddr     string //远端ip地址
	Port             int    //端口
	MatchCode        string //匹配子
	CardKeyCode      string //卡密要
	CalCardKet       string //开密钥
	CardMinBalance   int    //底金
	DayLimetMoney    int    //限额
	CommEncryptKey   string //传输密钥
	CardBatchEnable  string //卡批次
	PurseEnable      string //钱包
	ConsumeMode      int    //模式
	CardSector       int    //公共扇区
	CardEnableMonths int    //黑名单有效月
	CardRebate       int    //卡折扣
}

//设备信息
var deviceMsg DeviceMsgSt

/*==================================================================================
* 函 数 名： write_plate_read_msg_from_config_sqlite
* 参    数：
* 功能描述:  从配置数据库读取设备信息
* 返 回 值：
* 备    注： None
* 作    者： lc
* 创建时间： 2021/5/20
==================================================================================*/
func pos_read_msg_from_config_sqlite() {
	// var cdevMsg C.C_DevMsgSt
	// var cmenuMsg C.C_menuMsgSt

	//从配置数据库读取设备信息
	cdevMsg := C.sqlite_read_devMsg_from_config_db()
	deviceMsg.Version = C.GoString(cdevMsg.version)                 //Version
	deviceMsg.Maincode = int(cdevMsg.maincode)                      //设备号
	deviceMsg.LocalIpaddr = C.GoString(cdevMsg.localIpaddr)         //本机ip地址
	deviceMsg.RemoteIpaddr = C.GoString(cdevMsg.remoteIpaddr)       //远端ip地址
	deviceMsg.Port = int(cdevMsg.port)                              //端口号
	deviceMsg.MatchCode = C.GoString(cdevMsg.matchCode)             //匹配字
	deviceMsg.CardKeyCode = C.GoString(cdevMsg.cardKeyCode)         //卡密要
	deviceMsg.CalCardKet = C.GoString(cdevMsg.calCardKey)           //卡密要
	deviceMsg.CardMinBalance = int(cdevMsg.cardMinBalance)          //卡底金
	deviceMsg.DayLimetMoney = int(cdevMsg.dayLimetMoney)            //日限额
	deviceMsg.CommEncryptKey = C.GoString(cdevMsg.commEncryptKey)   //通讯密钥
	deviceMsg.CardBatchEnable = C.GoString(cdevMsg.cardBatchEnable) //卡批次
	deviceMsg.PurseEnable = C.GoString(cdevMsg.purseEnable)         //钱包
	deviceMsg.ConsumeMode = int(cdevMsg.consumeMode)                //消费模式
	deviceMsg.CardSector = int(cdevMsg.cardSector)                  //公共扇区
	deviceMsg.CardEnableMonths = int(cdevMsg.cardEnableMonths)      //黑名单有效月
	deviceMsg.CardRebate = int(cdevMsg.cardRebate)                  //卡折扣

	debug_printf("deviceMsg.Version = %s\n", deviceMsg.Version)
	debug_printf("设备号 = %d\n", deviceMsg.Maincode)
	debug_printf("本机IP = %s\n", deviceMsg.LocalIpaddr)
	debug_printf("远端IP = %s\n", deviceMsg.RemoteIpaddr)
	debug_printf("端口号 = %d\n", deviceMsg.Port)
	debug_printf("匹配字 = %s\n", deviceMsg.MatchCode)
	debug_printf("读卡密钥 = %s\n", deviceMsg.CardKeyCode)
	debug_printf("写卡密钥 = %s\n", deviceMsg.CalCardKet)
	debug_printf("卡底金 = %d\n", deviceMsg.CardMinBalance)
	debug_printf("卡限额 = %d\n", deviceMsg.DayLimetMoney)
	debug_printf("传输密钥 = %s\n", deviceMsg.CommEncryptKey)
	debug_printf("卡批次 = %s\n", deviceMsg.CardBatchEnable)
	debug_printf("钱包号 = %s\n", deviceMsg.PurseEnable)
	debug_printf("消费模式 = %d\n", deviceMsg.ConsumeMode)
	debug_printf("公共扇区 = %d\n", deviceMsg.CardSector)
	debug_printf("黑名单有效月 = %d\n", deviceMsg.CardEnableMonths)
	debug_printf("卡折扣 = %d\n", deviceMsg.CardRebate)

	//
	C.udp_read_devMsg_from_config_db()
	//从配置数据库读取卡的配置信息
	C.read_card_data_form_config_db()
	//从配置数据库读取消费信息
	C.read_consume_data_form_config_db()
}

/*==================================================================================
* 函 数 名： pos_app_init
* 参    数：
* 功能描述:  设备初始化函数
* 返 回 值：
* 备    注： None
* 作    者： lc
* 创建时间： 2021/4/14
==================================================================================*/
func pos_app_init() {

	//创建配置数据库
	//C.sqlite_create_config_db()
	//创建消费记录数据库
	//C.sqlite3_consume_open_db()

	//创建qt进程通讯消息队列
	C.create_qt_msg()
	//C.sqlite_read_devMsg_from_config_db()
	pos_read_msg_from_config_sqlite()
	//创建黑名单数据库
	//C.sqlite3_blaknumber_open_db()
	//创建C多任务
	C.app_wrplate_create_thread()
	//udp_client_test()
	//创建UDP连接
	udp_client_task(deviceMsg.LocalIpaddr, deviceMsg.RemoteIpaddr, deviceMsg.Port)
}
