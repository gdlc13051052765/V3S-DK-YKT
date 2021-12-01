package main

/*
#include <stdio.h>
#include "sqliteTask.h"
#include "msgTask.h"
#cgo CFLAGS: -I./
#cgo LDFLAGS: -L./ -lsqlite3  -lm
*/
import "C"

//设备信息
type DeviceMsgSt struct {
	Version         string //版本号
	Maincode        int    //设备号
	Ipaddr          string //ip地址
	Port            int    //端口
	MatchCode       string //匹配子
	CardKeyCode     string //卡密要
	CalCardKet      string //开密钥
	CardMinBalance  int    //底金
	DayLimetMoney   int    //限额
	CommEncryptKey  string //传输密钥
	CardBatchEnable string //卡批次
	PurseEnable     string //钱包
	ConsumeMode     int    //模式
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
	//var cdevMsg C.C_DevMsgSt
	// var cmenuMsg C.C_menuMsgSt

	//从配置数据库读取设备信息
	cdevMsg := C.sqlite_read_devMsg_from_config_db()
	deviceMsg.Version = C.GoString(cdevMsg.version)                 //Version
	deviceMsg.Maincode = int(cdevMsg.maincode)                      //设备号
	deviceMsg.Ipaddr = C.GoString(cdevMsg.ipaddr)                   //ip地址
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

	debug_printf("deviceMsg.Version = %s\n", deviceMsg.Version)
	debug_printf("deviceMsg.Maincode = %d\n", deviceMsg.Maincode)
	debug_printf("deviceMsg.Ipaddr = %s\n", deviceMsg.Ipaddr)
	debug_printf("deviceMsg.Port = %d\n", deviceMsg.Port)
	debug_printf("deviceMsg.MatchCode = %s\n", deviceMsg.MatchCode)
	debug_printf("deviceMsg.CardKeyCode = %s\n", deviceMsg.CardKeyCode)
	debug_printf("deviceMsg.CalCardKet = %s\n", deviceMsg.CalCardKet)
	debug_printf("deviceMsg.CardMinBalance = %d\n", deviceMsg.CardMinBalance)
	debug_printf("deviceMsg.DayLimetMoney = %d\n", deviceMsg.DayLimetMoney)
	debug_printf("deviceMsg.CommEncryptKey = %s\n", deviceMsg.CommEncryptKey)
	debug_printf("deviceMsg.CardBatchEnable = %s\n", deviceMsg.CardBatchEnable)
	debug_printf("deviceMsg.PurseEnable = %s\n", deviceMsg.PurseEnable)
	debug_printf("deviceMsg.ConsumeMode = %d\n", deviceMsg.ConsumeMode)

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

	//创建qt进程通讯消息队列
	C.create_qt_msg()
	//C.sqlite_read_devMsg_from_config_db()
	pos_read_msg_from_config_sqlite()
	//创建黑名单数据库
	C.sqlite3_blaknumber_open_db()
	//创建C多任务
	//C.app_wrplate_create_thread()

}
