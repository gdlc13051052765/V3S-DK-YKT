package main

import (
	"fmt"
	"net"
)

/*==================================================================================
* 函 数 名： udp_client_test
* 参    数：
* 功能描述:  udp 客户端发送测试函数
* 返 回 值： None
* 备    注：
* 作    者： lc
* 创建时间： 2021-05-25
==================================================================================*/
func main() {
	// 创建监听
	socket, err := net.ListenUDP("udp4", &net.UDPAddr{
		IP:   net.IPv4(192, 168, 3, 37),
		Port: 8080,
	})
	if err != nil {
		fmt.Println("监听失败!", err)
		return
	}
	defer socket.Close()

	for {
		// 读取数据
		data := make([]byte, 4096)
		read, remoteAddr, err := socket.ReadFromUDP(data)
		if err != nil {
			fmt.Println("读取数据失败!", err)
			continue
		}
		fmt.Println(read, remoteAddr)
		fmt.Printf("%s\n\n", data)

		// 发送数据
		senddata := []byte("hello client!")
		_, err = socket.WriteToUDP(senddata, remoteAddr)
		if err != nil {
			fmt.Println("发送数据失败!", err)
			return
		}
	}
}
