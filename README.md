# esp8266_smartcard

Use esp8266's GPIO to simulate a smart card,
while running an oscam client,
connecting to the cccam server via esp8266's wifi.

So as to achieve the set-top box DVB CW sharing.



使用esp8266来模拟智能卡，把esp8266接到标准的机顶盒读卡器上，从而实现cccam共享。
要用到四根数据线连接读卡器和esp8266的gpio端口。
分别是vcc、gnd、5、2。
gpio 2 为io，连接到到读卡器io
gpio 5 为rst，连接到到读卡器rst