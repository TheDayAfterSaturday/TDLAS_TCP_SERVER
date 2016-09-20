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

static int panelHandle; //�������
static int TabPanel_1;  //tabpanel handle
static int TabPanel_2;
static int TabPanel_3;

#define SERVER_PORT_NUM    60100
#define SERVER_PORT_CH1    60200   //����ͨ��1����
#define SERVER_PORT_CH2    60300   //����ͨ��2����
#define SERVER_IP         "223.3.40.114"

//SYS control command list for TCP
#define SOC_POWER           0x11
#define TEC_MON_START       0x12
#define TEC_MON_STOP        0x13
#define TEC_ON              0x14
#define TEC_OFF             0x15 
#define LASER_ION           0x16
#define LASER_IOFF          0x17
#define SET_NOTICE          0x1A //�������¶����á��������������ó�������
#define TEC_TSET            0x20
#define LASER_ISET          0x21 //DDS Config
#define AD_START            0x18 
#define AD_STOP             0x19
#define LASER_IWAVE         0x30
#define TEC_MONITOR_DAT     0x31

#define CMD_HEAD            0xA5A5
#define CMD_CREATE(CMD_HEAD,HEAD_LEN,HEAD_CMD) ((CMD_HEAD & 0xffff) <<16) | ((HEAD_LEN & 0xff) << 8) | ( HEAD_CMD & 0xff)


typedef struct DDS_Param //DDS ����
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

//TEC_MON��������
typedef struct monitorParam //TEC_MON������
{	//9��������   
	double laserRealI;
	double tecRealV;
	double tecLimitV;
	double tecRealI;
	double laserPower;
	double laserLimitI;
	double techeatLimitI;
	double teccoolLimitI;
	double laserTemp;
}monitorParam,monitorParamPtr; 

typedef struct paramSettingLimit //�������¶����á�����������������ֵ
{
	double tecTmax;
	double tecTmin;
	double laserImax;
}paramSettingLimit,paramSettingLimitPtr;

paramSettingLimit paramLimit; //�����ȫ�ֱ���   
//�����ʽ����
unsigned int Tec_TSET = 0xA5A50C20;	// CMD_CREATE(CMD_HEAD,sizeof(int)+sizeof(double),TEC_TSET); 
unsigned int Laser_ON = 0xA5A50416; // CMD_CREATE(CMD_HEAD,sizeof(int),LASER_ION);
unsigned int Laser_OFF = 0xA5A50417;// CMD_CREATE(CMD_HEAD,sizeof(int),LASER_IOFF);   
unsigned int Tec_ON = 0xA5A50414;   // CMD_CREATE(CMD_HEAD,sizeof(int),TEC_ON);
unsigned int Tec_OFF = 0xA5A50415;  // CMD_CREATE(CMD_HEAD,sizeof(int),TEC_OFF); 
unsigned int Tec_Mon_Start = 0xA5A50412; // CMD_CREATE(CMD_HEAD,sizeof(int),TEC_MON_START);
unsigned int Tec_Mon_Stop = 0xA5A50413;  // CMD_CREATE(CMD_HEAD,sizeof(int),TEC_MON_STOP);
unsigned int Laser_Iset = 0xA5A56421;    // CMD_CREATE(CMD_HEAD,sizeof(int)+sizeof(DDS_Param),LASER_ISET);
unsigned int AD_Sample_Start = 0xA5A50518;  // CMD_CREATE(CMD_HEAD,sizeof(int)+sizeof(char),AD_START);
unsigned int AD_Sample_Stop = 0xA5A50419;   // CMD_CREATE(CMD_HEAD,sizeof(int),AD_STOP);
unsigned int TEC_Monitor_Dat = 0xA5A54831;  // CMD_CREATE(CMD_HEAD,sizeof(int) + sizeof(monitorParam),TEC_MONITOR_DAT); 
unsigned int SET_Notice = 0xA5A51C1A;  //��������
unsigned int Get_ParamLimit = 0xA5A5041A; //��ȡ��ֵ��������

//TCP handle
static unsigned int g_hconversation = 0;  //��������������Ӧ�˿�60100
static unsigned int ch1_hconversation = 0; //ͨ��1�������ݾ������Ӧ�˿�60200
static unsigned int ch2_hconversation = 0; //ͨ��2�������ݾ������Ӧ�˿�60300
static int          g_TCPError = 0;
static int Server_registed = 0;

//User Interface goble variable
static unsigned int receiveCmd = 0; 

//TCP Receive Data and Write it in Disk
#define PACKET_SIZE  1024  //���ݰ��Ĵ�С 1024����
#define PACKET_NUM   1024  //����ͨ���ڴ��洢���ݰ�������ÿPACKET_NUM�����ݰ�ת��һ�ν���buffer
#define DATA_SIZE    PACKET_SIZE*PACKET_NUM  //���������ݿ��С����λΪsizeof(int)

FILE *fp_ch1 = NULL;
char *file_name_ch1 = NULL;
char path_ch1[128]; //CH1���ݴ��·��

FILE *fp_ch2 = NULL;
char *file_name_ch2 = NULL;
char path_ch2[128]; //CH2���ݴ��·��  


unsigned int *ch1_buf1 = NULL;  //����ͨ��1ʵ������ָ��
unsigned int *ch1_buf2 = NULL;  
unsigned int *ch1_save1 = NULL;  //ͨ��1д���ļ���ʵ�����ݵ�ָ��          
unsigned int *ch1_save2 = NULL;
unsigned int *ch1_ptr;  //�����ж�ʵ��������Ҫ�������ĸ��ڴ��
unsigned int *ch1_DataPtr;


unsigned int *ch2_buf1 = NULL;  //����ͨ��1ʵ������ָ��
unsigned int *ch2_buf2 = NULL;  
unsigned int *ch2_save1 = NULL;  //ͨ��1д���ļ���ʵ�����ݵ�ָ��          
unsigned int *ch2_save2 = NULL;
unsigned int *ch2_ptr;  //�����ж�ʵ��������Ҫ�������ĸ��ڴ��
unsigned int *ch2_DataPtr;


//���պʹ���AD���������߳�
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
static int DisconnectClient (unsigned int handle);
static int CVICALLBACK ServerCallback_Cmd (unsigned int handle, int xType, 
                                       int errCode, void *cbData);
static int CVICALLBACK ServerCallback_CH1 (unsigned int handle, int xType, 
                                       int errCode, void *cbData);
static int CVICALLBACK ServerCallback_CH2 (unsigned int handle, int xType, 
                                       int errCode, void *cbData);
static void ReportTCPError (int error);

//������
int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "TDLAS_TCP_SERVER.uir", PANEL)) < 0)
		return -1;
	
	//��ʼ��Tab�ؼ�����
	SetActiveTabPage (panelHandle, PANEL_TAB, 0);  
	GetPanelHandleFromTabPage (panelHandle, PANEL_TAB, 0, &TabPanel_1);  //��ȡTabHandle        
	GetPanelHandleFromTabPage (panelHandle, PANEL_TAB, 1, &TabPanel_2);  
	GetPanelHandleFromTabPage (panelHandle, PANEL_TAB, 2, &TabPanel_3); 
	
	//��ʼ����״ָ̬ʾ�� 
	SetCtrlAttribute (panelHandle, PANEL_TCP_LED, ATTR_OFF_COLOR,VAL_RED); 		//TCP����״ָ̬ʾ�ƣ���δ���ӵ�ʱ����ʾ��ɫ 
	SetCtrlAttribute (panelHandle, PANEL_TCP_LED, ATTR_ON_COLOR,VAL_GREEN); 	//TCP����״ָ̬ʾ�ƣ������ӵ�ʱ����ʾ��ɫ    
	SetCtrlAttribute (panelHandle, PANEL_CH1_LED, ATTR_OFF_COLOR,VAL_RED); 		//TCP����״ָ̬ʾ�ƣ���δ���ӵ�ʱ����ʾ��ɫ 
	SetCtrlAttribute (panelHandle, PANEL_CH1_LED, ATTR_ON_COLOR,VAL_GREEN); 	//TCP����״ָ̬ʾ�ƣ������ӵ�ʱ����ʾ��ɫ   
	SetCtrlAttribute (panelHandle, PANEL_CH2_LED, ATTR_OFF_COLOR,VAL_RED); 		//TCP����״ָ̬ʾ�ƣ���δ���ӵ�ʱ����ʾ��ɫ 
	SetCtrlAttribute (panelHandle, PANEL_CH2_LED, ATTR_ON_COLOR,VAL_GREEN); 	//TCP����״ָ̬ʾ�ƣ������ӵ�ʱ����ʾ��ɫ   
	
	ch1_buf1 = malloc(sizeof(int)*DATA_SIZE); //����1024*4*1024Byte��ԼΪ4MByte
	ch1_buf2 = malloc(sizeof(int)*DATA_SIZE);
	
	ch1_save1 = ch1_buf1;
	ch1_save2 = ch1_buf2; 
	
	ch2_buf1 = malloc(sizeof(int)*DATA_SIZE); //  
	ch2_buf2 = malloc(sizeof(int)*DATA_SIZE);
	
	ch2_save1 = ch2_buf1;
	ch2_save2 = ch2_buf2; 
	
	//�����ź���
	Semaphore_handle1 = CreateSemaphore(0,0,1000000,0);
	Semaphore_handle2 = CreateSemaphore(0,0,1000000,0); 
	
	DisplayPanel (panelHandle);
	RunUserInterface ();
	DiscardPanel (panelHandle);
	return 0;
}

//�������ݣ�
//CH1: �˿�60200 --- ch1_hconversation
DWORD WINAPI ThreadFunction1_ch1(LPVOID param)
{
	int messageSize = DATA_SIZE; //1024*1024����
	int wordsToRead;
	int wordsRead;
	while(startflag == 1) 
	{
		ch1_ptr = ch1_buf1;  //���ݿ��ŵ�ch1_buf
		wordsToRead = messageSize;
		while(ch1_hconversation == 0); //�ȴ�����
		while(wordsToRead > 0) //ÿ�ζ�ȡ���������Ĵ�С�������������˿����ݻ�������С�йأ�����д��ÿ�ζ�ȡ1024���ֵ�����(PACKET_SIZE)
		{
			wordsRead = ServerTCPRead(ch1_hconversation,&ch1_ptr[messageSize - wordsToRead],PACKET_SIZE*sizeof(int),1000)/sizeof(int); //ServerTCPRead����ֵ�Ƕ�ȡ����byte��
			wordsToRead -= wordsRead;
			//printf("wordsRead = %d\n",wordsRead);
		}
		ReleaseSemaphore(Semaphore_handle1,1,NULL); //�ź�������ֵ��1��ʹ��ThreadFunction2_ch1���ch1_buf1������д���ı��ļ�
		
		wordsToRead = messageSize;
		ch1_ptr = ch1_buf2;
		while(wordsToRead > 0)
		{
			wordsRead = ServerTCPRead(ch1_hconversation,&ch1_ptr[messageSize - wordsToRead],PACKET_SIZE*sizeof(int),1000)/sizeof(int); //ServerTCPRead����ֵ�Ƕ�ȡ����byte��
			wordsToRead -= wordsRead;
			//printf("wordsRead = %d\n",wordsRead);     
		}
		ReleaseSemaphore(Semaphore_handle1,1,NULL); //�ź�������ֵ��1��ʹ��ThreadFunction2_ch1���ch1_buf2������д���ı��ļ�  		
	}
	return 0;
}

//��ͨ��1������д���ı��ļ�ch1_data.dat��
DWORD WINAPI ThreadFunction2_ch1(LPVOID param)
{
	while(startflag==1)
	{
		ch1_DataPtr = ch1_save1; 
		WaitForSingleObject(Semaphore_handle1,INFINITE); //�ȴ�ch1_buf1�����һ�����ݰ�
		fwrite(ch1_DataPtr,sizeof(int),DATA_SIZE,fp_ch1);   //��ch1_buf1������д�뵽�ı��ļ�ch1_data.dat��
		
		ch1_DataPtr = ch1_save2;
		WaitForSingleObject(Semaphore_handle1,INFINITE); //�ȴ�ch1_buf2�����һ�����ݰ�
		fwrite(ch1_DataPtr,sizeof(int),DATA_SIZE,fp_ch1);  //��ch1_buf2������д�뵽�ı��ļ�ch1_data.dat��
	}
	return 0;
}

//��������
//CH2���˿�60300 --- ch2_hconversation  
DWORD WINAPI ThreadFunction1_ch2(LPVOID param)
{
	int messageSize = DATA_SIZE; //1024*4096����     
	int wordsToRead;
	int wordsRead;
	while(startflag == 1)
	{
		ch2_ptr = ch2_buf1;  //����һ�����ݰ���ŵ�ch2_buf 
		wordsToRead = messageSize;
		while(ch2_hconversation == 0); //�ȴ�����  
		while(wordsToRead > 0) 
		{
			wordsRead = ServerTCPRead(ch2_hconversation,&ch2_ptr[messageSize - wordsToRead],PACKET_SIZE*sizeof(int),1000)/sizeof(int); //ServerTCPRead����ֵ�Ƕ�ȡ����byte��
			wordsToRead -= wordsRead;
		//	printf("wordsRead = %d\n",wordsRead);     
		}
		ReleaseSemaphore(Semaphore_handle2,1,NULL); //�ź�������ֵ��1��ʹ��ThreadFunction2_ch2���ch2_buf1������д���ı��ļ�
		
		ch2_ptr = ch2_buf2;
		wordsToRead = messageSize;  
		while(wordsToRead > 0)
		{
			wordsRead = ServerTCPRead(ch2_hconversation,&ch2_ptr[messageSize - wordsToRead],PACKET_SIZE*sizeof(int),1000)/sizeof(int); //ServerTCPRead����ֵ�Ƕ�ȡ����byte��
			wordsToRead -= wordsRead;
		//	printf("wordsRead = %d\n",wordsRead);     
		}
		ReleaseSemaphore(Semaphore_handle2,1,NULL); //�ź�������ֵ��1��ʹ��ThreadFunction2_ch2���ch2_buf2������д���ı��ļ�   	
	}
	return 0;
}

//ͨ��2����д���ı��ļ�ch2_data.dat
DWORD WINAPI ThreadFunction2_ch2(LPVOID param)
{
	while(startflag==1)
	{
		ch2_DataPtr = ch2_save1; 
		WaitForSingleObject(Semaphore_handle2,INFINITE); //�ȴ�ch2_buf1�����һ�����ݰ�
		fwrite(ch2_DataPtr,sizeof(int),DATA_SIZE,fp_ch2);   //��ch2_buf1������д�뵽�ı��ļ�ch2_data.dat��
		
		ch2_DataPtr = ch2_save2;
		WaitForSingleObject(Semaphore_handle2,INFINITE); //�ȴ�ch2_buf2�����һ�����ݰ�
		fwrite(ch2_DataPtr,sizeof(int),DATA_SIZE,fp_ch2);  //��ch2_buf2������д�뵽�ı��ļ�ch2_data.dat��
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

//ע��������
int  CVICALLBACK UnregisterServer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event)
	{
		case EVENT_COMMIT:
			if(Server_registed == 1)
			{
				if(ConfirmPopup("Message Confirm","ȷ��ע��?"))
				{
					Server_registed = 0;
					UnregisterTCPServer(SERVER_PORT_NUM);	
					UnregisterTCPServer(SERVER_PORT_CH1);
					UnregisterTCPServer(SERVER_PORT_CH2);
					SetCtrlVal(panelHandle,PANEL_TCP_LED,0);
				}
			}
			else
				MessagePopup("Warning","TCP Serverδע�ᣡ");
	}
	return 0;
}

//TCP Server����
int CVICALLBACK Tcp_Config (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:  //TCP Serverע��
			if(Server_registed == 1)
				MessagePopup("Message","TCP Server�����ע��!"); 
			else 
			{	
				if(
					//(RegisterTCPServerEx(SERVER_PORT_NUM,ServerCallback_Cmd,NULL,SERVER_IP)<0) ||;
					(RegisterTCPServer (SERVER_PORT_NUM, ServerCallback_Cmd, NULL) < 0) ||
				    (RegisterTCPServer (SERVER_PORT_CH1, ServerCallback_CH1, NULL) < 0) ||
				    (RegisterTCPServer (SERVER_PORT_CH2, ServerCallback_CH2, NULL) < 0) 
				  )	
				{
					MessagePopup("Error","TCP Serverע��ʧ��!");
					Server_registed = 0;
			    }
				else 
				{
				   	Server_registed = 1;
					MessagePopup("Message","TCP Serverע��ɹ����ȴ�Client����!");
				}
			}
			break;
	}
	return 0;
}

/***********************************************************************************************************/
//TCP Server�ص�����: ��λ����3��socket��Ӧ3��Client��1������������λ�������������������AD��������
/* ServerCallback_Cmd: ��������
 * ServerCallback_CH1: ����ͨ��1������
 * ServerCallback_CH2: ����ͨ��2������
 */
static int CVICALLBACK ServerCallback_Cmd (unsigned int handle, int xType, 
                                       int errCode, void *cbData)
{
	switch(xType)
	{
		case TCP_CONNECT:   //������Client��������
			g_hconversation = handle; //��������
			SetCtrlVal(panelHandle,PANEL_TCP_LED,1); 
			ServerTCPWrite(g_hconversation,&Get_ParamLimit,sizeof(Get_ParamLimit),1000); //���ͻ�ȡ��ֵ��������:0x1A   
			break;
		case TCP_DISCONNECT: //Client�Ͽ�����
			g_hconversation = handle;
			DisconnectClient (g_hconversation);
			g_hconversation = 0;
			break;
		case TCP_DATAREADY:  //Server�˿�������:��λ���ط�������
		    g_hconversation = handle;
		    ServerTCPRead(g_hconversation,&receiveCmd,sizeof(receiveCmd),1000); //��ȡ����
		    switch(receiveCmd)
		    {
		    	case 0xA5A50C20://Tec_TSET:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    	
		    	case 0xA5A50416://Laser_ON:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A50417://Laser_OFF:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A50414://Tec_ON:
		     		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A50415://Tec_OFF:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A50412://Tec_Mon_Start: //�ȴ����ռ������
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A50413://Tec_Mon_Stop:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A56421://Laser_Iset:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    	
		    	case 0xA5A50518://AD_Sample_Start:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		break;
		    		
		    	case 0xA5A50419://AD_Sample_Stop:
		    		printf("receiveCmd = %x\n",receiveCmd);
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_1,FALSE);
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_2,FALSE);         
					SetCtrlVal(TabPanel_3,TABPANEL_3_SAMPLE_LED,FALSE);   //�رղ���״ָ̬ʾ��
					while(1)	//���߳�
					{  
						startflag = 0;  //ֹͣ��������
						TerminateThread(thread1_ch1,exitCode1_ch1);
						TerminateThread(thread2_ch1,exitCode2_ch1); 
						TerminateThread(thread1_ch2,exitCode1_ch2);
						TerminateThread(thread2_ch2,exitCode2_ch2);
						break;
					}
		    		break;
					
				case 0xA5A51C1A: //SET_Notice: �������¶����á��������������ó�������	
					printf("receiveCmd = %x\n",receiveCmd);
				//	paramSettingLimit paramLimit; //�����ȫ�ֱ���
					ServerTCPRead(g_hconversation,&paramLimit,sizeof(paramLimit),1000); //����Client����������ֵ����  
					//��ʾ������� 
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_TEMP_MAX,paramLimit.tecTmax);  
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_TEMP_MIN,paramLimit.tecTmin);
			    	SetCtrlVal(TabPanel_2,TABPANEL_2_LASER_CURREBT_MAX,paramLimit.laserImax);    
					break;
		    		
		    	case 0xA5A54831://TEC_Monitor_Dat:
		    		printf("receiveCmd = %x\n",receiveCmd);
		    		monitorParam monParam; //
		    		ServerTCPRead(g_hconversation,&monParam,sizeof(monParam),1000); //����Client�������ļ�����
					//��ʾ���ݵ����
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_REAL_V,monParam.tecRealV);
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_LIMIT_V,monParam.tecLimitV);
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_REAL_I,monParam.tecRealI);
					SetCtrlVal(TabPanel_1,TABPANEL_1_LASER_POWER,monParam.laserPower);
					SetCtrlVal(TabPanel_1,TABPANEL_1_LASER_LIMIT_I,monParam.laserLimitI);  
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_HLIMIT_I,monParam.techeatLimitI);
					SetCtrlVal(TabPanel_1,TABPANEL_1_TEC_CLIMIT_I,monParam.teccoolLimitI);
				
					//Laser Current & Laser Temperature
					SetCtrlVal(TabPanel_1,TABPANEL_1_LASER_TEMP,monParam.laserTemp);
					SetCtrlVal(TabPanel_1,TABPANEL_1_LASER_CURRENT,monParam.laserRealI);  
				
					//��ʾ���ݵ�Strip Chart��
					PlotStripChartPoint(TabPanel_1,TABPANEL_1_CURRENT_STRIP,monParam.laserRealI);  
					PlotStripChartPoint(TabPanel_1,TABPANEL_1_TEMP_STRIP,monParam.laserTemp); 
					break;
						
				default:
					MessagePopup("Error","�յ�δ���������");
					break;		    		
		    }
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
			SetCtrlVal(panelHandle,PANEL_CH1_LED,1);  
			break;
		case TCP_DISCONNECT: //Client�Ͽ�����
			ch1_hconversation = handle;
			DisconnectClient (ch1_hconversation);
			ch1_hconversation = 0;
			break;
		case TCP_DATAREADY:  //Server�˿�������---CH1������
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
		case TCP_CONNECT:   //������Client��������
			ch2_hconversation = handle;
			SetCtrlVal(panelHandle,PANEL_CH2_LED,1);
			break;
		case TCP_DISCONNECT: //Client�Ͽ�����
			ch2_hconversation = handle;
			DisconnectClient (ch2_hconversation);
			ch2_hconversation = 0;
			break;
		case TCP_DATAREADY:  //Server�˿�������---CH2������
		    ch2_hconversation = handle;
			break;
	}
    return 0;
}
/*********************************************************************************************/

//Client��Server�Ͽ����� 
static int DisconnectClient (unsigned int handle)
{
	//�ر�TCP����ָʾ��
	SetCtrlVal(panelHandle,PANEL_TCP_LED,1);	
	DisconnectTCPClient(handle);
	return 0;
}

int CVICALLBACK GetLimitParam (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch(event)
	{
		case EVENT_ACTIVE_TAB_CHANGE:
			ServerTCPWrite(g_hconversation,&Get_ParamLimit,sizeof(Get_ParamLimit),1000); //���ͻ�ȡ��ֵ��������:0x1A 
			break;
	}
	return 0;
}

//TEC�¶��趨
int CVICALLBACK SetTemperature (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double Tec_Temperature = 0.0;
	double ReceiveData = 0.0;  
	//double tecTmin = 0; //TEC�¶���ֵ
	//double tecTmax = 0;
	
	switch (event)
	{
		case EVENT_COMMIT:
		//	GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_TEMP_MIN,&tecTmin);  //��ȡTEC�¶����� 
	    //	GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_TEMP_MAX,&tecTmax);  //��ȡTEC�¶�����
			GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_TEMP,&Tec_Temperature); //��ȡTEC�趨�¶�
			if(Tec_Temperature < paramLimit.tecTmin)
				MessagePopup("Error","���õ��¶�ֵ�����������¶����ޣ�����������!");
			else if(Tec_Temperature > paramLimit.tecTmax)
				MessagePopup("Error","���õ��¶�ֵ�����������¶����ޣ�����������!");
			else
			{
				ServerTCPWrite(g_hconversation,&Tec_TSET,sizeof(Tec_TSET),1000); //����TEC_TSET����
				ServerTCPWrite(g_hconversation,&Tec_Temperature,sizeof(Tec_Temperature),1000); //��������
			}
			break;
	}
	return 0;
}

//Laser���ؿ���
int CVICALLBACK Laser_Ctrl (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int Laser_Ctrl_State = 0;  
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_1,TABPANEL_1_LASER_CTRL,&Laser_Ctrl_State);
			if(Laser_Ctrl_State == 1)
				ServerTCPWrite(g_hconversation,&Laser_ON,sizeof(Laser_ON),1000);
			else
				ServerTCPWrite(g_hconversation,&Laser_OFF,sizeof(Laser_OFF),1000);   
			break;
	}
	return 0;
}

//TEC ���ؿ���
int CVICALLBACK Tec_Ctrl (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int Tec_Ctrl_State = 0;
	switch (event)
	{
		case EVENT_COMMIT:
		  	GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_CTRL,&Tec_Ctrl_State);
			if(Tec_Ctrl_State == 1)
				ServerTCPWrite(g_hconversation,&Tec_ON,sizeof(Tec_ON),1000);
			else
				ServerTCPWrite(g_hconversation,&Tec_OFF,sizeof(Tec_OFF),1000);
			break;
	}
	return 0;
}

//TEC���ӿ���:ÿ��10s����һ֡����(9��double������)���������յ���������ʾ��������ʾ�������
int CVICALLBACK Tec_Mon_Ctrl (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	unsigned int Tec_Mon_State = 0;
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(TabPanel_1,TABPANEL_1_TEC_MON_CTRL,&Tec_Mon_State);
			if(Tec_Mon_State == 1)
				ServerTCPWrite(g_hconversation,&Tec_Mon_Start,sizeof(Tec_Mon_Start),1000);
			else  //Tec_Mon_Stop
				ServerTCPWrite(g_hconversation,&Tec_Mon_Stop,sizeof(Tec_Mon_Stop),1000);  //TEC_MON_STOP
			break;
	}
	return 0;
}

//DDS��������
int CVICALLBACK DDS_Config (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	DDS_Param DDS_Param; //DDS�����ṹ��
	//double laserImax = 0.0;
	double iMax = 0.0;
	
	switch (event)
	{
		case EVENT_COMMIT:
			//GetCtrlVal(TabPanel_2,TABPANEL_2_LASER_CURREBT_MAX,&laserImax); //��ȡ��������������      
			
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
			
			iMax = DDS_Param.dc2 + DDS_Param.amp_sin + DDS_Param.amp_saw; //��ȡ�趨�ļ������������ֵ
			if(iMax > paramLimit.laserImax)
				MessagePopup("Error","���õĵ�����ֵ���ڼ������������ޣ�����������!");
			else
			{
				ServerTCPWrite(g_hconversation,&Laser_Iset,sizeof(Laser_Iset),1000); //����Laser_Iset����
				ServerTCPWrite(g_hconversation,&DDS_Param,sizeof(DDS_Param),1000); //��������
			}
			break;
	}
	return 0;
}

//AD��������
int CVICALLBACK AD_Ctrl(int panel, int control, int event,
	    void *callbackData, int eventData1, int eventData2)
{
	unsigned int AD_CH1_State = 0;  //��Ӧ2������
	unsigned int AD_CH2_State = 0;
	unsigned int AD_State = 0;
	unsigned char channel_num = 0; //ͨ����
	unsigned int sendNum = 0;
	
	switch (event)
	{
		case EVENT_COMMIT:  //AD_CHAN_SET��ť������
			GetCtrlVal(TabPanel_3,TABPANEL_3_AD_CH1_CTRL,&AD_CH1_State); 
			GetCtrlVal(TabPanel_3,TABPANEL_3_AD_CH2_CTRL,&AD_CH2_State); 
			
			AD_State = AD_CH2_State;   //�ϲ��������ص�״̬
			AD_State = AD_State << 1;
			AD_State = AD_State + AD_CH1_State; //AD_State������int���ݺϲ��õ���
			
			switch(AD_State)
			{
				case 0: //AD����ֹͣ
					sendNum = ServerTCPWrite(g_hconversation,&AD_Sample_Stop,sizeof(AD_Sample_Stop),1000);	//����AD����ֹͣ���� 
					printf("sendNum = %d\n",sendNum);
					break;
					
				case 1: //ͨ��1��ʼ
					channel_num = 1;	
					ServerTCPWrite(g_hconversation,&AD_Sample_Start,sizeof(AD_Sample_Start),1000);	//����AD������ʼ���� 
					ServerTCPWrite(g_hconversation,&channel_num,sizeof(channel_num),1000); //����ͨ����
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_1,TRUE);
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_2,FALSE);         
					SetCtrlVal(TabPanel_3,TABPANEL_3_SAMPLE_LED,TRUE);   //��������״ָ̬ʾ��
					while(1)  //����2���̣߳���Ӧͨ��1���˿����ݣ�д�ı��ļ�
					{
						startflag = 1;
						thread1_ch1 = CreateThread(NULL,0,ThreadFunction1_ch1,NULL,0,NULL); 
						thread2_ch1 = CreateThread(NULL,0,ThreadFunction2_ch1,NULL,0,NULL); 
					
						GetExitCodeThread(thread1_ch1, &exitCode1_ch1);
						GetExitCodeThread(thread2_ch1, &exitCode2_ch1);
						break;
					}
					break;
					
				case 2: //ͨ��2��ʼ
					channel_num = 2; 	
					ServerTCPWrite(g_hconversation,&AD_Sample_Start,sizeof(AD_Sample_Start),1000);	//����AD������ʼ���� 
					ServerTCPWrite(g_hconversation,&channel_num,sizeof(channel_num),1000); //����ͨ����
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_1,FALSE);
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_2,TRUE);         
					SetCtrlVal(TabPanel_3,TABPANEL_3_SAMPLE_LED,TRUE);   //��������״ָ̬ʾ��
					while(1) //����2���̣߳���Ӧͨ��2�Ķ��˿����ݣ�д�ı��ļ�  
					{
						startflag = 1;
						thread1_ch2 = CreateThread(NULL,0,ThreadFunction1_ch2,NULL,0,NULL); 
						thread2_ch2 = CreateThread(NULL,0,ThreadFunction2_ch2,NULL,0,NULL);

						GetExitCodeThread(thread1_ch2, &exitCode1_ch2);
						GetExitCodeThread(thread2_ch2, &exitCode2_ch2);
						break;
					}
					break;
					
				case 3: //ͨ��1 2����ʼ
					channel_num = 3; //0000_0011	
					ServerTCPWrite(g_hconversation,&AD_Sample_Start,sizeof(AD_Sample_Start),1000);	//����AD������ʼ���� 
					ServerTCPWrite(g_hconversation,&channel_num,sizeof(channel_num),1000); //����ͨ����
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_1,TRUE);
					SetCtrlVal(TabPanel_3,TABPANEL_3_AD_LED_2,TRUE);         
					SetCtrlVal(TabPanel_3,TABPANEL_3_SAMPLE_LED,TRUE);   //��������״ָ̬ʾ��
					while(1) //����4���̣߳���Ӧͨ��1��2�Ķ��˿����ݣ�д�ı��ļ� 
					{
						startflag = 1;
						thread1_ch1 = CreateThread(NULL,0,ThreadFunction1_ch1,NULL,0,NULL); 
						thread2_ch1 = CreateThread(NULL,0,ThreadFunction2_ch1,NULL,0,NULL); 
					
						thread1_ch2 = CreateThread(NULL,0,ThreadFunction1_ch2,NULL,0,NULL);
						thread2_ch2 = CreateThread(NULL,0,ThreadFunction2_ch2,NULL,0,NULL);
						
						GetExitCodeThread(thread1_ch1, &exitCode1_ch1);
						GetExitCodeThread(thread2_ch1, &exitCode2_ch1);
						
						GetExitCodeThread(thread1_ch2, &exitCode1_ch2);
						GetExitCodeThread(thread2_ch2, &exitCode2_ch2);
						break;
					}
					break;
			}
			break;
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
			file_name_ch1 = strcat(path_ch1,"\\data_ch1.dat");//�趨�ļ���
			fp_ch1 = fopen(file_name_ch1,"ab+");  //���ļ�����ֵ���ļ�ָ�룬���ö�дģʽΪ:�ɶ���д��������ԭ������
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
			file_name_ch2 = strcat(path_ch2,"\\data_ch2.dat");//�趨�ļ���
			fp_ch2 = fopen(file_name_ch2,"ab+");  //���ļ�����ֵ���ļ�ָ�룬���ö�дģʽΪ:�ɶ���д��������ԭ������    
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
