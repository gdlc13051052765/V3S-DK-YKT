package main

import (
	"encoding/binary"
	"encoding/hex"
	"strings"
)

//字节转16进制字符串，并且字符转大写
func ByteToHex(buf []byte) string {
	return strings.ToUpper(hex.EncodeToString(buf))
}

func DataLength(buf []byte) uint16 {
	return binary.BigEndian.Uint16(inversion(buf[:2]))
}

//反转字节
func inversion(buf []byte) []byte {
	for i := 0; i < len(buf)/2; i++ {
		temp := buf[i]
		buf[i] = buf[len(buf)-1-i]
		buf[len(buf)-1-i] = temp
	}
	return buf
}
