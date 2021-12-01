package main

import (
	"encoding/hex"
	"fmt"
	"net"
	"time"
)

//udp socket 句柄
var pUdpSocket net.Conn

/*==================================================================================
* 函 数 名： udp_client_test
* 参    数：
* 功能描述:  udp 客户端发送测试函数
* 返 回 值： None
* 备    注：
* 作    者： lc
* 创建时间： 2021-05-25
==================================================================================*/
func init_udp_client(ipaddr string) {
	var err error
	pUdpSocket, err = net.Dial("udp", ipaddr)
	if err != nil {
		fmt.Println("连接失败!", err)
		return
	}
}

/*==================================================================================
* 函 数 名： udp_client_test
* 参    数：
* 功能描述:  udp 客户端发送测试函数
* 返 回 值： None
* 备    注：
* 作    者： lc
* 创建时间： 2021-05-25
==================================================================================*/
func udp_client_test(ipaddr string) {
	var err error
	pUdpSocket, err = net.Dial("udp", ipaddr)
	if err != nil {
		fmt.Println("连接失败!", err)
		return
	}
	//defer pUdpSocket.Close()
	//这里是设备正常请求
	//假设该设备序号
	SerialNumbe := "AA001D80"
	go func() {
		for {
			//报文长度占用俩个字节(报文长度不包含自身)，类型1个字节，设备序列号4个字节
			decodeString, err := hex.DecodeString("050000" + SerialNumbe)
			if err != nil {
				fmt.Println("错误的16进制字符串！")
			}
			fmt.Printf("发送的心跳包:{%s}\n", ByteToHex(decodeString))
			pUdpSocket.Write(decodeString)
			//心跳发送后，睡眠5秒
			time.Sleep(time.Duration(10) * time.Second)
		}
	}()
	buf := make([]byte, 1024)
	for {
		_, err := pUdpSocket.Read(buf)
		if err != nil {
			fmt.Println("conn.Read err:", err)
			return
		}
		fmt.Printf("收到服务器心跳包返回:{%s}\n", ByteToHex(buf[:len(buf)]))
		fmt.Print("请输入请求报文:\n")
	}
}

/*==================================================================================
* 函 数 名： udp_client_test
* 参    数：
* 功能描述:  udp 客户端发送测试函数
* 返 回 值： None
* 备    注：
* 作    者： lc
* 创建时间： 2021-05-25
==================================================================================*/
// func udp_client_test() {
// 	// 创建连接
// 	socket, err := net.DialUDP("udp4", nil, &net.UDPAddr{
// 		IP:   net.IPv4(192, 168, 3, 37),
// 		Port: 8080,
// 	})
// 	if err != nil {
// 		fmt.Println("连接失败!", err)
// 		return
// 	}
// 	defer socket.Close()

// 	// 发送数据
// 	senddata := []byte("hello server!")
// 	_, err = socket.Write(senddata)
// 	if err != nil {
// 		fmt.Println("发送数据失败!", err)
// 		return
// 	}

// 	// 接收数据
// 	data := make([]byte, 4096)
// 	read, remoteAddr, err := socket.ReadFromUDP(data)
// 	if err != nil {
// 		fmt.Println("读取数据失败!", err)
// 		return
// 	}
// 	fmt.Println(read, remoteAddr)
// 	fmt.Printf("%s\n", data)
// }
