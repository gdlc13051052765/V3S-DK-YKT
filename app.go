package main

/*
#include <stdio.h>
#include "sqliteTask.h"
#include "msgTask.h"
#include "udp_data_analyse.h"
#include "debug_print.h"
#include "CalucationFile.h"
#include "sysTime.h"
#include "VariableDef.h"
#cgo CFLAGS: -I./
#cgo LDFLAGS: -L./ -lsqlite3  -lm
*/
import "C"
import (
	"fmt"
	"time"
)

var version string = "YKT_APP_0.0.1"

/*==================================================================================
                      全局变量
==================================================================================*/

//主流程
func main() {
	fmt.Printf("MCT写盘器软件版本 == %s\n", version)
	//初始化
	pos_app_init()

	//udp_client_test()
	udp_client_test("192.168.3.37:8080")

	for {
		//主任务
		time.Sleep(1 * time.Second)
	}
}
