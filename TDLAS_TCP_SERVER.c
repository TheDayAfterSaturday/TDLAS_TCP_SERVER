#include <tcpsupp.h>
#include <Windows.h>
#include <userint.h> 
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <utility.h>
#include <errno.h>
#include <ansi_c.h>
#include <cvirte.h>	
#include "TDLAS_TCP_SERVER.h"

static int panelHandle; //主面板句柄
static int TabPanel_1;  //tabpanel handle
static int TabPanel_2;
static int TabPanel_3;

#define SERVER_PORT_NUM    60100
#define SERVER_PORT_CH1    60200   //接收通道1数据
#define SERVER_PORT_CH2    60300   //接收通道2数据
#define SERVER_IP         "223.3.54.191"

//SYS control command list for TCP
#define SOC_POWER           0x11
#define TEC_MON_START       0x12
#define TEC_MON_STOP        0x13
#define TEC_ON              0x14
#define TEC_OFF             0x15 
#define LASER_ION           0x16
#define LASER_IOFF          0x17
#define TEC_TSET            0x20
#define LASER_ISET          0x21 //DDS Config
#define AD_START            0x18 
#define AD_STOP             0x19
#define LASER_IWAVE         0x30
#define TEC_MONITOR_DAT     0x31
#define LASER_TWAVE         0x80  //自定义

#define CMD_HEAD            0xA5A5
#define CMD_CREATE(CMD_HEAD,HEAD_LEN,HEAD_CMD) ((CMD_HEAD & 0xffff) <<16) | ((HEAD_LEN & 0xff) << 8) | ( HEAD_CMD & 0xff)


typedef struct DDS_Param
{
	double  freq_sq1x;
	double  phase_sq1x;
	double  freq_sq2x;
	double  phase_sq2x;
	double  freq_sin;
	double  phase_sin;
	double  amp_sin;
	double  freq_saw;
	double  amp_saw;
	double  dc1;
	double  dc2;
	double  delay_saw;
	unsigned int outen;
} DDS_Param,*DDS_ParamPtr;

//TCP handle
static unsigned int g_hconversation = 0;  //发送命令句柄，对应端口60100
static unsigned int ch1_hconversation = 0; //通道1接收数据句柄，对应端口60200
static unsigned int ch2_hconversation = 0; //通道2接收数据句柄，对应端口60300
static int          g_TCPError = 0;
static int Server_registed = 0;

//User Interface goble variable
static unsigned int receiveCmd = 0; 

FILE *fp_ch1 = NULL;
char *file_name_ch1 = NULL;
char path_ch1[128]; //CH1数据存放路径

FILE *fp_ch2 = NULL;
char *file_name_ch2 = NULL;
char path_ch2[128]; //CH2数据存放路径

#define PACKET_SIZE  1024  //数据包的大小 1024个字

unsigned int *ch1_buf1 = NULL;  //缓存通道1实验数据指针
unsigned int *ch1_buf2 = NULL;  
unsigned int *ch1_save1 = NULL;  //通道1写入文件的实验数据的指针          
unsigned int *ch1_save2 = NULL;
unsigned int *ch1_ptr;  //用于判断实验数据需要缓存在哪个内存块
unsigned int *ch1_DataPtr;


unsigned int *ch2_buf1 = NULL;  //缓存通道1实验数据指针
unsigned int *ch2_buf2 = NULL;  
unsigned int *ch2_save1 = NULL;  //通道1写入文件的实验数据的指针          
unsigned int *ch2_save2 = NULL;
unsigned int *ch2_ptr;  //用于判断实验数据需要缓存在哪个内存块
unsigned int *ch2_DataPtr;


//接收和储存AD采样数据线程
static int startflag=0;
HANDLE thread1_ch1 = NULL,thread2_ch1 = NULL;
HANDLE thread1_ch2 = NULL,thread2_ch2 = NULL;
DWORD WINAPI ThreadFunction1_ch1(LPVOID param);
DWORD WINAPI ThreadFunction2_ch1(LPVOID param);
DWORD WINAPI ThreadFunction1_ch1(LPVOID param);
DWORD WINAPI ThreadFunction2_ch1(LPVOID param);

DWORD exitCode1_ch1 = 0;
DWORD exitCode2_ch1 = 0;
DWORD exitCode1_ch2 = 0;
DWORD exitCode2_ch2 = 0;

HANDLE  Semaphore_handle1; 
HANDLE  Semaphore_handle2;

/*---------------------------------------------------------------------------*/
/* Internal function prototypes                                              */
/*---------------------------------------------------------------------------*/
static int ConnectClient (unsigned int handle);
static int DisconnectClient (unsigned int handle);
static int CVICALLBACK ServerCallback_Cmd (unsigned int handle, int xType, 
                                       int errCode, void *cbData);
static int CVICALLBACK ServerCallback_CH1 (unsigned int handle, int xType, 
                                       int errCode, void *cbData);
static int CVICALLBACK ServerCallback_CH2 (unsigned int handle, int xType, 
                                       int errCode, void *cbData);
static void ReportTCPError (int error);

//主函数
int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "TDLAS_TCP_SERVER.uir", PANEL)) < 0)
		return -1;
	
	//初始化Tab控件活动面板
	SetActiveTabPage (panelHandle, PANEL_TAB, 0);  
	GetPanelHandleFromTabPage (panelHandle, PANEL_TAB, 0, &TabPanel_1);  //获取TabHandle        
	GetPanelHandleFromTabPage (panelHandle, PANEL_TAB, 1, &TabPanel_2);  
	GetPanelHandleFromTabPage (panelHandle, PANEL_TAB, 2, &TabPanel_3); 
	
	//初始化各状态指示灯 
	SetCtrlAttribute (panelHandle, PANEL_TCP_LED, ATTR_OFF_COLOR,VAL_RED); 		//TCP连接状态指示灯，在未连接的时候显示红色 
	SetCtrlAttribute (panelHandle, PANEL_TCP_LED, ATTR_ON_COLOR,VAL_GREEN); 	//TCP连接状态指示灯，在连接的时候显示绿色    
	
	SetCtrlAttribute (panelHandle, PANEL_TEC_MON_TIMER, ATTR_ENABLED,0);         //关闭定时器  
	
	ch1_buf1 = malloc(sizeof(int)*PACKET_SIZE); //分配1024*4*48Byte
	ch1_buf2 = malloc(sizeof(int)*PACKET_SIZE);
	
	ch1_save1 = ch1_buf1;
	ch1_save2 = ch1_buf2; 
	
	ch2_buf1 = malloc(sizeof(int)*PACKET_SIZE); //分配1024*4Byte
	ch2_buf2 = malloc(sizeof(int)*PACKET_SIZE);
	
	ch2_save1 = ch2_buf1;
	ch2_save2 = ch2_buf2; 
	
	//创建信号量
	Semaphore_handle1 = CreateSemaphore(0,0,1000000,0);
	Semaphore_handle2 = CreateSemaphore(0,0,1000000,0); 
	
	DisplayPanel (panelHandle);
	RunUserInterface ();
	DiscardPanel (panelHandle);
	return 0;
}

//接收数据：
//CH1: 端口60200 --- ch1_hconversation
DWORD WINAPI ThreadFunction1_ch1(LPVOID param)
{
	int messageSize = PACKET_SIZE;
	int wordsToRead;
	int wordsRead;
	while(startflag == 1) 
	{
		ch1_ptr = ch1_buf1;  //接收一个数据包存放到ch1_buf 
		wordsToRead = messageSize;
		while(wordsToRead > 0) 
		{
			wordsRead = ServerTCPRead(ch1_hconversation,&ch1_ptr[messageSize - wordsToRead],wordsToRead,1000)/sizeof(int); //ServerTCPRead返回值是读取数据byte数
			wordsToRead -= wordsRead;
		}
		ReleaseSemaphore(Semaphore_handle1,1,NULL); //信号量计数值加1，使得ThreadFunction2_ch1完成ch1_buf1的数据写入文本文件
		
		wordsToRead = messageSize;
		while(wordsToRead > 0)
		{
			wordsRead = ServerTCPRead(ch1_hconversation,&ch1_ptr[messageSize - wordsToRead],wordsToRead,1000)/sizeof(int); //ServerTCPRead返回值是读取数据byte数
			wordsToRead -= wordsRead;
		}
		ReleaseSemaphore(Semaphore_handle1,1,NULL); //信号量计数值加1，使得ThreadFunction2_ch1完成ch1_buf2的数据写入文本文件  		
	}
	return 0;
}

//将通道1的数据写到文本文件ch1_data.dat中
DWORD WINAPI ThreadFunction2_ch1(LPVOID param)
{
	while(startflag==1)
	{
		ch1_DataPtr = ch1_save1; 
		WaitForSingleObject(Semaphore_handle1,INFINITE); //等待ch1_buf1缓存好一个数据包
		fwrite(ch1_DataPtr,1,PACKET_SIZE,fp_ch1);   //将ch1_buf1中数据写入到文本文件ch1_data.dat中
		
		ch1_DataPtr = ch1_save2;
		WaitForSingleObject(Semaphore_handle1,INFINITE); //等待ch1_buf2缓存好一个数据包
		fwrite(ch1_DataPtr,1,PACKET_SIZE,fp_ch1);  //将ch1_buf2中数据写入到文本文件ch1_data.dat中
	}
	return 0;
}

//接收数据
//CH2：端口60300 --- ch2_hconversation  
DWORD WINAPI ThreadFunction1_ch2(LPVOID param)
{
	int messageSize = PACKET_SIZE;
	int wordsToRead;
	int wordsRead;
	while(startflag == 1)
	{
		ch2_ptr = ch2_buf1;  //接收一个数据包存放到ch2_buf 
		wordsToRead = messageSize;
		while(wordsToRead > 0) 
		{
			wordsRead = ServerTCPRead(ch2_hconversation,&ch2_ptr[messageSize - wordsToRead],wordsToRead,1000)/sizeof(int); //ServerTCPRead返回值是读取数据byte数
			wordsToRead -= wordsRead;
		}
		ReleaseSemaphore(Semaphore_handle2,1,NULL); //信号量计数值加1，使得ThreadFunction2_ch2完成ch2_buf1的数据写入文本文件
		
		wordsToRead = messageSize;
		while(wordsToRead > 0)
		{
			wordsRead = ServerTCPRead(ch2_hconversation,&ch2_ptr[messageSize - wordsToRead],wordsToRead,1000)/sizeof(int); //ServerTCPRead返回值是读取数据byte数
			wordsToRead -= wordsRead;
		}
		ReleaseSemaphore(Semaphore_handle2,1,NULL); //信号量计数值加1，使得ThreadFunction2_ch2完成ch2_buf2的数据写入文本文件   	
	}
	return 0;
}

//通道2数据写入文本文件ch2_data.dat
DWORD WINAPI ThreadFunction2_ch2(LPVOID param)
{
	while(startflag==1)
	{
		ch2_DataPtr = ch2_save1; 
		WaitForSingleObject(Semaphore_handle2,INFINITE); //等待ch2_buf1缓存好一个数据包
		fwrite(ch2_DataPtr,1,PACKET_SIZE,fp_ch2);   //将ch2_buf1中数据写入到文本文件ch2_data.dat中
		
		ch2_DataPtr = ch2_save2;
		WaitForSingleObject(Semaphore_handle2,INFINITE); //等待ch2_buf2缓存好一个数据包
		fwrite(ch2_DataPtr,1,PACKET_SIZE,fp_ch2);  //将ch2_buf2中数据写入到文本文件ch2_data.dat中
	}
	return 0;
}

int CVICALLBACK panelCB (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{									  
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			QuitUserInterface (0);
			break;
	}
	return 0;
}

//TCP Server配置
int CVICALLBACK Tcp_Config (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:  //TCP Server注册
			if(Server_registed == 1)
				MessagePopup("Message","TCP Server已完成注册!"); 
			else 
			{	
				//if(RegisterTCPServer (SERVER_PORT_NUM, ServerCallback, NULL) < 0) 
				if(
					(RegisterTCPServer (SERVER_PORT_NUM, ServerCallback_Cmd, NULL) < 0) ||
				    (RegisterTCPServer (SERVER_PORT_CH1, ServerCallback_CH1, NULL) < 0) ||
				    (RegisterTCPServer (SERVER_PORT_CH2, ServerCallback_CH2, NULL) < 0) 
				  )	
				{
					MessagePopup("Error","TCP Server注册失败!");
					Server_registed = 0;
			    }
				else 
				{
				   	Server_registed = 1;
					MessagePopup("Message","TCP Server注册成功，等待Client连接!");
				}
			}
			break;
	}
	return 0;
}

/***********************************************************************************************************/
//TCP Server回调函数: 下位机有3个socket对应3个Client，1个用来接收上位机命令，另两个用来传递AD采样数据
/* ServerCallback_Cmd: 发送命令
 * ServerCallback_CH1: 接收通道1的数据
 * ServerCallback_CH2: 接收通道2的数据
 */
static int CVICALLBACK ServerCallback_Cmd (unsigned int handle, int xType, 
                                       int errCode, void *cbData)
{
	switch(xType)
	{
		case TCP_CONNECT:   //允许多个Client申请连接
			g_hconversation = handle;
			ConnectClient(g_hconversation);
			break;
		case TCP_DISCONNECT: //Client断开连接
			g_hconversation = handle;
			DisconnectClient (g_hconversation);
			g_hconversation = 0;
			break;
		case TCP_DATAREADY:  //Server端口有数据
		    g_hconversation = handle;
			break;
	}
    return 0;
}

static int CVICALLBACK ServerCallback_CH1 (unsigned int handle, int xType, 
                                       int errCode, void *cbData)
{
	switch(xType)
	{
		case TCP_CONNECT:   
			ch1_hconversation = handle;
			ConnectClient(ch1_hconversation);
			break;
		case TCP_DISCONNECT: //Client断开连接
			ch1_hconversation = handle;
			DisconnectClient (ch1_hconversation);
			ch1_hconversation = 0;
			break;
		case TCP_DATAREADY:  //Server端口有数据---CH1有数据
		    ch1_hconversation = handle;   
			break;
	}
    return 0;
}

static int CVICALLBACK ServerCallback_CH2 (unsigned int handle, int xType, 
                                       int errCode, void *cbData)
{
	switch(xType)
	{
		case TCP_CONNECT:   //允许多个Client申请连接
			ch2_hconversation = handle;
			ConnectClient(ch2_hconversation);
			break;
		case TCP_DISCONNECT: //Client断开连接
			ch2_hconversation = handle;
			DisconnectClient (ch2_hconversation);
			ch2_hconversation = 0;
			break;
		case TCP_DATAREADY:  //Server端口有数据---CH2有数据
		    ch2_hconversation = handle;
			break;
	}
    return 0;
}
/*********************************************************************************************/

//Client与Server建立连接
static int ConnectClient (unsigned int handle)
{
    int             tcpErr = 0;
    char            peerAddress[128];
   
	//获取Client的IP地址
    GetTCPPeerAddr (handle, peerAddress, sizeof (peerAddress));
    //点亮TCP连接指示灯
	SetCtrlVal(panelHandle,PANEL_TCP_LED,1);
	printf("Client Handle = %d\n",handle); //打印handle	  
    return 0;
}

//Client与Server断开连接 
static int DisconnectClient (unsigned int handle)
{
	//关闭TCP连接指示灯
	SetCtrlVal(panelHandle,PANEL_TCP_LED,1);	
	DisconnectTCPClient(handle);
	return 0;
}


//TEC温度设定
int CVICALLBACK SetTemperature (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double Tec_Temperature = 0.0;
	double ReceiveData = 0.0;  
	unsigned int head_len = 4 + sizeof(Tec_Temperature);
	unsigned int Tec_TSET = CMD_CREATE(CMD_HEAD,head_len,TEC_TSET); //生成格式化命令
	
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_TEMP,&Tec_Temperature);
			ServerTCPWrite(g_hconversation,&Tec_TSET,sizeof(Tec_TSET),1000); //发送TEC_TSET命令
			ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令
			printf("Client Reback Cmd = %x\n",receiveCmd); //打印   
			if(receiveCmd == Tec_TSET)
			{
				ServerTCPWrite(g_hconversation,&Tec_Temperature,sizeof(Tec_Temperature),1000); //发送数据
		    	ServerTCPRead(g_hconversation,&ReceiveData,sizeof(ReceiveData),1000); //接收回发的数据
				printf("Tec_Temperature = %lf\n",ReceiveData); //打印  
			}
			else
				MessagePopup("Error","发送命令与接收到的回发命令不匹配!");
			break;
	}
	return 0;
}

//读电源参数
int CVICALLBACK ReadPowerParam (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{  
	unsigned int head_len = 4;
	unsigned int Soc_Power = CMD_CREATE(CMD_HEAD,head_len,SOC_POWER); //生成格式化命令
	switch (event)
	{
		case EVENT_COMMIT:
			ServerTCPWrite(g_hconversation,&Soc_Power,sizeof(Soc_Power),1000); //发送SOC_POWER命令
			ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
			printf("Clent Reback Cmd = %x\n",receiveCmd);
			break;
	}
	return 0;
}

//Laser开关控制
int CVICALLBACK Laser_Ctrl (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int head_len = 4;
	unsigned int Laser_Ctrl_State = 0;
	unsigned int Laser_ON = CMD_CREATE(CMD_HEAD,head_len,LASER_ION);
	unsigned int Laser_OFF = CMD_CREATE(CMD_HEAD,head_len,LASER_IOFF);        
	
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_1,TABPANEL_1_LASER_CTRL,&Laser_Ctrl_State);
			if(Laser_Ctrl_State == 1)
			{
				ServerTCPWrite(g_hconversation,&Laser_ON,sizeof(Laser_ON),1000);
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
			    printf("Clent Reback Cmd = %x\n",receiveCmd);
			}
			else
			{
				ServerTCPWrite(g_hconversation,&Laser_OFF,sizeof(Laser_OFF),1000);   
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
				printf("Clent Reback Cmd = %x\n",receiveCmd);
			}
			break;
	}
	return 0;
}

//TEC 开关控制
int CVICALLBACK Tec_Ctrl (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int head_len = 4;
	unsigned int Tec_Ctrl_State = 0;
	unsigned int Tec_ON = CMD_CREATE(CMD_HEAD,head_len,TEC_ON);
	unsigned int Tec_OFF = CMD_CREATE(CMD_HEAD,head_len,TEC_OFF); 
	switch (event)
	{
		case EVENT_COMMIT:
		  	GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_CTRL,&Tec_Ctrl_State);
			if(Tec_Ctrl_State == 1)
			{
				ServerTCPWrite(g_hconversation,&Tec_ON,sizeof(Tec_ON),1000);
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
				printf("Clent Reback Cmd = %x\n",receiveCmd);
			}
			else
			{
				ServerTCPWrite(g_hconversation,&Tec_OFF,sizeof(Tec_OFF),1000);
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
				printf("Clent Reback Cmd = %x\n",receiveCmd);	
			}
			break;
	}
	return 0;
}

//TEC监视控制
int CVICALLBACK Tec_Mon_Ctrl (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int head_len = 4;
	unsigned int Tec_Mon_State = 0;
	unsigned int Tec_Mon_Start = CMD_CREATE(CMD_HEAD,head_len,TEC_MON_START);
	unsigned int Tec_Mon_Stop = CMD_CREATE(CMD_HEAD,head_len,TEC_MON_STOP);
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_MON_CTRL,&Tec_Mon_State);
			if(Tec_Mon_State == 1)
			{
				ServerTCPWrite(g_hconversation,&Tec_Mon_Start,sizeof(Tec_Mon_Start),1000);
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
				printf("Clent Reback Cmd = %x\n",receiveCmd);
				if(receiveCmd == Tec_Mon_Start)
					SetCtrlAttribute(panelHandle,PANEL_TEC_MON_TIMER,ATTR_ENABLED,1);  //开启1s定时器      
				else
					MessagePopup("Error","发送命令与接收到的回发命令不匹配!"); 
			}
			else   
			{
				ServerTCPWrite(g_hconversation,&Tec_Mon_Stop,sizeof(Tec_Mon_Stop),1000); 
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令	
				printf("Clent Reback Cmd = %x\n",receiveCmd);
				if(receiveCmd == Tec_Mon_Stop)
					SetCtrlAttribute(panelHandle,PANEL_TEC_MON_TIMER,ATTR_ENABLED,0); //关闭定时器
				else
					MessagePopup("Error","发送命令与收到的回发命令不匹配!");
			}
			break;
	}
	return 0;
}

//TEC监视定时器：每隔1s读取激光器温度参数、电流参数并显示到面板上
int CVICALLBACK Mon_timer(int panel, int control, int event, 
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int head_len = 4; //sizeof(int)
	unsigned int LaserI_Wave = CMD_CREATE(CMD_HEAD,head_len,LASER_IWAVE); //获取激光器电流的命令：0xA5A50430
	unsigned int LaserT_Wave = CMD_CREATE(CMD_HEAD,head_len,LASER_TWAVE); //获取激光器温度的命令: 0xA5A50480
	double laser_Temperature = 20.0;
	double laser_Current = 20.0;
	switch(event)
	{
		case EVENT_TIMER_TICK: //1s定时
			ServerTCPWrite(g_hconversation,&LaserI_Wave,sizeof(LaserI_Wave),1000); //发送命令LASER_IWAVE
			ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000);
			if(receiveCmd == LaserI_Wave)
			{
				ServerTCPRead(g_hconversation,&laser_Current,sizeof(laser_Current),1000); //接收电流数据
				SetCtrlVal(TabPanel_1,TABPANEL_1_LASER_CURRENT,laser_Current);  //数据显示
				PlotStripChartPoint(TabPanel_1,TABPANEL_1_CURRENT_STRIP,laser_Current);      
			}
			else
				MessagePopup("Error","发送命令与收到的回发命令不匹配!");  
			
			ServerTCPWrite(g_hconversation,&LaserT_Wave,sizeof(LaserT_Wave),1000); //发送命令LASER_TWAVE
			ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000);
			if(receiveCmd == LaserT_Wave)
			{
				ServerTCPRead(g_hconversation,&laser_Temperature,sizeof(laser_Temperature),1000); //接收温度数据
				SetCtrlVal(TabPanel_1,TABPANEL_1_LASER_TEMP,laser_Temperature);  //数据显示
				PlotStripChartPoint(TabPanel_1,TABPANEL_1_TEMP_STRIP,laser_Temperature);      
			}
			else
				MessagePopup("Error","发送命令与收到的回发命令不匹配!");  
			break;
	}
	return 0;
}


//DDS参数配置
int CVICALLBACK DDS_Config (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	DDS_Param DDS_Param; //DDS参数结构体
	unsigned int head_len = 4 + sizeof(DDS_Param);
	unsigned int DDS_Config = CMD_CREATE(CMD_HEAD,head_len,LASER_ISET);
	
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_2,TABPANEL_2_FREQ_SQ1X,&DDS_Param.freq_sq1x); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_PHASE_SQ1X,&DDS_Param.phase_sq1x); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_FREQ_SQ2X,&DDS_Param.freq_sq2x); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_PHASE_SQ2X,&DDS_Param.phase_sq2x); 
			
			GetCtrlVal(TabPanel_2,TABPANEL_2_FREQ_SIN,&DDS_Param.freq_sin); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_PHASE_SIN,&DDS_Param.phase_sin); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_AMP_SIN,&DDS_Param.amp_sin); 
			
			GetCtrlVal(TabPanel_2,TABPANEL_2_FREQ_SAW,&DDS_Param.freq_saw); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_AMP_SAW,&DDS_Param.amp_saw);
			GetCtrlVal(TabPanel_2,TABPANEL_2_DC_1,&DDS_Param.dc1); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_DC_2,&DDS_Param.dc2); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_DELAY_SAW,&DDS_Param.delay_saw); 
			GetCtrlVal(TabPanel_2,TABPANEL_2_OUT_EN,&DDS_Param.outen); 
			
			ServerTCPWrite(g_hconversation,&DDS_Config,sizeof(DDS_Config),1000); //发送TEC_TSET命令
			ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //接收Client回发的命令
			printf("Clent Reback Cmd = %x\n",receiveCmd);
			if(receiveCmd == DDS_Config)
			{
				ServerTCPWrite(g_hconversation,&DDS_Param,sizeof(DDS_Param),1000); //发送数据
			}
			else
				MessagePopup("Error","发送命令与接收到的回发命令不匹配!");
			
			break;
	}
	return 0;
}

//AD采样控制
int CVICALLBACK AD_Ctrl(int panel, int control, int event,
	    void *callbackData, int eventData1, int eventData2)
{
	unsigned int AD_CH1_State = 0;  //对应3个开光
	unsigned int AD_CH2_State = 0;
	unsigned int AD_Ctrl_State = 0;
	unsigned int head_len = 4;
	unsigned int AD_Sample_Start = CMD_CREATE(CMD_HEAD,head_len,AD_START);
	unsigned int AD_Sample_Stop = CMD_CREATE(CMD_HEAD,head_len,AD_STOP);
	
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_3,TABPANEL_3_AD_CH1_CTRL,&AD_CH1_State); 
			GetCtrlVal(TabPanel_3,TABPANEL_3_AD_CH2_CTRL,&AD_CH2_State); 
			GetCtrlVal(TabPanel_3,TABPANEL_3_AD_CTRL,&AD_Ctrl_State);  //总开关
			if(AD_Ctrl_State && AD_CH1_State && AD_CH2_State)  //启动AD采样
			{
				ServerTCPWrite(g_hconversation,&AD_Sample_Start,sizeof(AD_Sample_Start),1000);	//发送AD采样开始命令
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000);
				printf("Clent Reback Cmd = %x\n",receiveCmd);    
				if(receiveCmd == AD_Sample_Start)
				{
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_SAMPLE_LED,TRUE);  //点亮AD采样状态指示灯
					while(1)
					{
						startflag = 1;
						thread1_ch1 = CreateThread(NULL,0,ThreadFunction1_ch1,NULL,0,NULL); //创建4个线程，对应通道1和2的读端口数据，写文本文件
						thread2_ch1 = CreateThread(NULL,0,ThreadFunction2_ch1,NULL,0,NULL); 
						
						thread1_ch2 = CreateThread(NULL,0,ThreadFunction1_ch2,NULL,0,NULL); //创建4个线程，对应通道1和2的读端口数据，写文本文件
						thread2_ch2 = CreateThread(NULL,0,ThreadFunction2_ch2,NULL,0,NULL);
						GetExitCodeThread(thread1_ch1, &exitCode1_ch1);
						GetExitCodeThread(thread2_ch1, &exitCode2_ch1);
						GetExitCodeThread(thread1_ch2, &exitCode1_ch2);
						GetExitCodeThread(thread2_ch2, &exitCode2_ch2);
						break;
					}	
				}
				else
					MessagePopup("Error","发送命令与接收到的回发命令不匹配!");
				
			}
			else   //停止AD采样
			{
				ServerTCPWrite(g_hconversation,&AD_Sample_Stop,sizeof(AD_Sample_Stop),1000);   //发送AD采样停止命令
				ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000);
				printf("Clent Reback Cmd = %x\n",receiveCmd);
				if(receiveCmd == AD_Sample_Stop)
				{
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_SAMPLE_LED,FALSE);   //关闭采样状态指示灯
					while(1)
					{  
						startflag = 0;
						TerminateThread(thread1_ch1,exitCode1_ch1);
						TerminateThread(thread2_ch1,exitCode2_ch1); 
						TerminateThread(thread1_ch2,exitCode1_ch2);
						TerminateThread(thread2_ch2,exitCode2_ch2);
						break;
					}
				}
				else
					MessagePopup("Error","发送命令与接收到的回发命令不匹配!");
			}
	}
	return 0;
}

int CVICALLBACK Browser_CH1 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char DirPath[512];
	switch (event)
	{
		case EVENT_COMMIT:
			DirSelectPopup ("D:\\DATA", "Select Directory", 1, 1, DirPath);
			SetCtrlVal(TabPanel_3, TABPANEL_3_CH1_PATH, DirPath);
			strcpy(path_ch1,DirPath);
			file_name_ch1 = strcat(path_ch1,"\\data_ch1.dat");//设定文件名
			fp_ch1 = fopen(file_name_ch1,"ab+");  //打开文件，赋值给文件指针，设置读写模式为:可读可写，不覆盖原有数据
			break;
	}
	return 0;
}

int CVICALLBACK Browser_CH2 (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char DirPath[512];
	switch (event)
	{
		case EVENT_COMMIT:
			DirSelectPopup ("D:\\DATA", "Select Directory", 1, 1, DirPath);
			SetCtrlVal(TabPanel_3, TABPANEL_3_CH2_PATH, DirPath);
			strcpy(path_ch2,DirPath);
			file_name_ch2 = strcat(path_ch2,"\\data_ch2.dat");//设定文件名
			fp_ch2 = fopen(file_name_ch2,"ab+");  //打开文件，赋值给文件指针，设置读写模式为:可读可写，不覆盖原有数据    
			break;
	}
	return 0;
}

//TCP Error Report
static void ReportTCPError (int error)
{
    char    messageBuffer[1024];

    if (error < 0)
        {
        sprintf(messageBuffer, 
            "TCP library error message: %s\nSystem error message: %s", 
            GetTCPErrorString (error), GetTCPSystemErrorString());
        MessagePopup ("Error", messageBuffer);
        }
}
