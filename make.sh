rm -rf posApp
CGO_ENABLED=1 GOOS=linux GOARCH=arm GOARM=5 CC=/media/user1/V3SSDK_BUILDROOT/v3ssdk/buildroot/out/host/bin/arm-buildroot-linux-gnueabihf-gcc go build

