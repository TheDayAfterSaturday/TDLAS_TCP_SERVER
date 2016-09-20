/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2016. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: panelCB */
#define  PANEL_QUIT                       2       /* control type: command, callback function: UnregisterServer */
#define  PANEL_TAB                        3       /* control type: tab, callback function: GetLimitParam */
#define  PANEL_TCP_LED                    4       /* control type: LED, callback function: (none) */
#define  PANEL_TCP_CONFIG                 5       /* control type: command, callback function: Tcp_Config */
#define  PANEL_CH2_LED                    6       /* control type: LED, callback function: (none) */
#define  PANEL_CH1_LED                    7       /* control type: LED, callback function: (none) */

     /* tab page panel controls */
#define  TABPANEL_1_TEC_TEMP_MAX          2       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_TEMP_MIN          3       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_TEMP              4       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEMP_SET              5       /* control type: command, callback function: SetTemperature */
#define  TABPANEL_1_LASER_LIMIT_I         6       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_REAL_I            7       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_REAL_V            8       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_CLIMIT_I          9       /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_HLIMIT_I          10      /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEC_LIMIT_V           11      /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_8             12      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_7             13      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_4             14      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_DECORATION            15      /* control type: deco, callback function: (none) */
#define  TABPANEL_1_LASER_CTRL            16      /* control type: binary, callback function: Laser_Ctrl */
#define  TABPANEL_1_TEC_CTRL              17      /* control type: binary, callback function: Tec_Ctrl */
#define  TABPANEL_1_TEC_MON_CTRL          18      /* control type: binary, callback function: Tec_Mon_Ctrl */
#define  TABPANEL_1_LASER_TEMP            19      /* control type: scale, callback function: (none) */
#define  TABPANEL_1_LASER_CURRENT         20      /* control type: scale, callback function: (none) */
#define  TABPANEL_1_TEMP_STRIP            21      /* control type: strip, callback function: (none) */
#define  TABPANEL_1_LASER_POWER           22      /* control type: numeric, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_9             23      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_2             24      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_3             25      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_5             26      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_CURRENT_STRIP         27      /* control type: strip, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_6             28      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_12            29      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_DECORATION_2          30      /* control type: deco, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_11            31      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_TEXTMSG_10            32      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_1_DECORATION_3          33      /* control type: deco, callback function: (none) */
#define  TABPANEL_1_TEXTMSG               34      /* control type: textMsg, callback function: (none) */

     /* tab page panel controls */
#define  TABPANEL_2_PHASE_SQ2X            2       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_LASER_CURREBT_MAX     3       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_FREQ_SQ2X             4       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_PHASE_SQ1X            5       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_DELAY_SAW             6       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_DC_2                  7       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_DC_1                  8       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_AMP_SAW               9       /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_FREQ_SAW              10      /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_AMP_SIN               11      /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_PHASE_SIN             12      /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_FREQ_SIN              13      /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_FREQ_SQ1X             14      /* control type: numeric, callback function: (none) */
#define  TABPANEL_2_TEXTMSG               15      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_8             16      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_2             17      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_3             18      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_5             19      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_6             20      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_10            21      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_9             22      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_OUT_EN                23      /* control type: binary, callback function: (none) */
#define  TABPANEL_2_DDS_CONFIG            24      /* control type: command, callback function: DDS_Config */
#define  TABPANEL_2_DECORATION            25      /* control type: deco, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_13            26      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_4             27      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_DECORATION_2          28      /* control type: deco, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_7             29      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_14            30      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_DECORATION_3          31      /* control type: deco, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_11            32      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_12            33      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_16            34      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_DECORATION_4          35      /* control type: deco, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_15            36      /* control type: textMsg, callback function: (none) */
#define  TABPANEL_2_DECORATION_5          37      /* control type: deco, callback function: (none) */
#define  TABPANEL_2_TEXTMSG_17            38      /* control type: textMsg, callback function: (none) */

     /* tab page panel controls */
#define  TABPANEL_3_AD_CH2_CTRL           2       /* control type: binary, callback function: (none) */
#define  TABPANEL_3_AD_CH1_CTRL           3       /* control type: binary, callback function: (none) */
#define  TABPANEL_3_AD_LED_2              4       /* control type: LED, callback function: (none) */
#define  TABPANEL_3_AD_LED_1              5       /* control type: LED, callback function: (none) */
#define  TABPANEL_3_CH2_PATH              6       /* control type: string, callback function: (none) */
#define  TABPANEL_3_BROWSER_CH2           7       /* control type: command, callback function: Browser_CH2 */
#define  TABPANEL_3_CH1_PATH              8       /* control type: string, callback function: (none) */
#define  TABPANEL_3_BROWSER_CH1           9       /* control type: command, callback function: Browser_CH1 */
#define  TABPANEL_3_AD_CTRL_SET           10      /* control type: command, callback function: AD_Ctrl */
#define  TABPANEL_3_SAMPLE_LED            11      /* control type: LED, callback function: (none) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK AD_Ctrl(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Browser_CH1(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Browser_CH2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK DDS_Config(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK GetLimitParam(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Laser_Ctrl(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK panelCB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetTemperature(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Tcp_Config(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Tec_Ctrl(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Tec_Mon_Ctrl(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK UnregisterServer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
