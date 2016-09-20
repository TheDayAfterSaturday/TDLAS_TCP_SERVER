TDLAS_TCP_SERVER(09-13)工程：
	本工程采用两个Buffer缓存数据，在Semphore机制下采用ping-pong操作完成数据读取和回写文本，遇到的问题是：Buffer2中数据可能无法回写到本地。
TDLAS_TCP_SERVER(09-18)工程：
	(1) 基本功能部分已调通。可能要求将检测数据部分和阈值部分的显示控件：Numeric Edit控件(Indicator Mode)换成String Edit控件(Indicator Mode)。我感觉没必要换。
	(2) 可能存在Bebug的地方是多线程读写数据时，Semphore信号量的控制。
