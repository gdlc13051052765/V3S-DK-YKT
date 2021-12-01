rm -rf posApp
CGO_ENABLED=1 GOOS=linux GOARCH=arm GOARM=5 CC=arm-linux-gnueabihf-gcc go build

