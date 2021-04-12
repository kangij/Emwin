
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "GUI.h"
#include "mtb_st7789v.h"
#include "cy8ckit_028_tft_pins.h" /* This is part of the CY8CKIT-028-TFT shield library. */
//#include "cy_retarget_io.h"

#include "tft_lcd.h"
#include "FreeRtos.h"
#include "task.h"
#include "timers.h"
#include "list.h"
#include "message_buffer.h"
#include "queue.h"
#include "semphr.h"

#include "common.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmmerge;
extern GUI_CONST_STORAGE GUI_BITMAP bmfeatured_image;

/* Pointer to the new frame that need to be written */
uint8_t *current_frame;
uint32_t Graph_Limit_count=0;

/********************************************************************************/
 //* 					EMWIN TEXT POSITION SETTING
// ********************************************************************************/

#define TEMP_STR_POS_X		110
#define TEMP_STR_POS_Y		50

// GYRO Variable X/Y/Z
#define GYRO_STR_POS_X		110
#define GYRO_STR_POS_Y		150

#define HUMI_STR_POS_X		280
#define HUMI_STR_POS_Y		50

// GYRO Variable X/Y/Z
#define ACCEL_STR_POS_X		280
#define ACCEL_STR_POS_Y		150

#define GUI_STR_VAL_OFFSET_X 20
#define GUI_STR_VAL_OFFSET_Y 20

/********************************************************************************/
 //* 					EMWIN TFTLCD DATA
// ********************************************************************************/
#include "DIALOG.h"

typedef struct _GRAPH_Function
{
	GRAPH_SCALE_Handle hScale;
	WM_HWIN            hGraph;
	GRAPH_DATA_Handle  hData[4]; // 0: temp 1: humi 2:light 3:motion
}GRAPH_Function;

GRAPH_Function Graph_func;
QueueHandle_t TFT_LCD_CommandQ;


/*******************************************************************************
* Macros
*******************************************************************************/
#define DELAY_AFTER_STARTUP_SCREEN_MS       (2000)
#define AMBIENT_TEMPERATURE_C               (20)
#define SPI_BAUD_RATE_HZ                    (20000000)

/*******************************************************************************
* Forward declaration
*******************************************************************************/
static void Show_startup_screen(void);
static void Show_Logo(void);
static void Show_Text_Window(void);
static void Show_Graph_Window(void);
static void Show_Text_Window_Update(void);
static void Show_Graph_Window_Update(void);

extern GUI_CONST_STORAGE GUI_BITMAP bmmerge;
extern GUI_CONST_STORAGE GUI_BITMAP bmfeatured_image;

/*******************************************************************************
* Function Name: void show_startup_screen(void)
********************************************************************************/
const mtb_st7789v_pins_t tft_pins =
{
    .db08 = CY8CKIT_028_TFT_PIN_DISPLAY_DB8,
    .db09 = CY8CKIT_028_TFT_PIN_DISPLAY_DB9,
    .db10 = CY8CKIT_028_TFT_PIN_DISPLAY_DB10,
    .db11 = CY8CKIT_028_TFT_PIN_DISPLAY_DB11,
    .db12 = CY8CKIT_028_TFT_PIN_DISPLAY_DB12,
    .db13 = CY8CKIT_028_TFT_PIN_DISPLAY_DB13,
    .db14 = CY8CKIT_028_TFT_PIN_DISPLAY_DB14,
    .db15 = CY8CKIT_028_TFT_PIN_DISPLAY_DB15,
    .nrd  = CY8CKIT_028_TFT_PIN_DISPLAY_NRD,
    .nwr  = CY8CKIT_028_TFT_PIN_DISPLAY_NWR,
    .dc   = CY8CKIT_028_TFT_PIN_DISPLAY_DC,
    .rst  = CY8CKIT_028_TFT_PIN_DISPLAY_RST
};

static void Emwin_Driver_Setup(void)
{
    cy_rslt_t result;
    result = mtb_st7789v_init8(&tft_pins);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
	GUI_Init();
}

static void Show_startup_screen(void)
{
    /* Set foreground and background color and font size */
    GUI_SetFont(GUI_FONT_16B_1);
    GUI_SetColor(GUI_BLACK);
    GUI_SetBkColor(GUI_WHITE);
    GUI_Clear();

    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_DispStringAt("PRESS CAPSENSE TOUCH", 132, 85);
}

/*******************************************************************************
* Function Name: void show_font_sizes_normal(void)
********************************************************************************
*
* Summary: This function shows various font sizes
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void Show_Logo(void)
{
    /* Draw the Cypress Logo */
    GUI_DrawBitmap(&bmfeatured_image, 0, 0);

    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_SetTextMode(GUI_TEXTMODE_REV);
    GUI_SetColor(GUI_WHITE);
    GUI_DispStringAt("IOT SYSTEM Project", 200, 200);
    GUI_SetColor(GUI_BLACK);

    /* Print the text Cypress EMWIN GRAPHICS DEMO TFT DISPLAY */
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_SetTextMode(GUI_TEXTMODE_REV);
    GUI_SetColor(GUI_WHITE);
    GUI_DispStringAt("V0.0.1", 220, 215);
    GUI_SetColor(GUI_BLACK);

}


/*******************************************************************************
* Function Name: void show_font_sizes_bold(void)
********************************************************************************
*
* Summary: This function shows various font sizes
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
static void Show_Text_Window(void)
{
    /* Draw the Cypress Logo */
    GUI_DrawBitmap(&bmmerge, 0, 0);

    // Temparature
    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_SetTextMode(GUI_TEXTMODE_REV);
    GUI_SetColor(GUI_WHITE);
    GUI_DispStringAt("TEMP:", TEMP_STR_POS_X, TEMP_STR_POS_Y);
    GUI_SetColor(GUI_BLACK);

    // Gyro
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("LIGHT:", GYRO_STR_POS_X, GYRO_STR_POS_Y+(GUI_STR_VAL_OFFSET_Y*1));
	GUI_SetColor(GUI_BLACK);


	// Humid
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("HUMI:", HUMI_STR_POS_X, HUMI_STR_POS_Y);
	GUI_SetColor(GUI_BLACK);

	// Motion
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("X:", ACCEL_STR_POS_X, ACCEL_STR_POS_Y);
	GUI_SetColor(GUI_BLACK);
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("Y:", ACCEL_STR_POS_X, ACCEL_STR_POS_Y+(GUI_STR_VAL_OFFSET_Y*1));
	GUI_SetColor(GUI_BLACK);
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt("Z:", ACCEL_STR_POS_X, ACCEL_STR_POS_Y+(GUI_STR_VAL_OFFSET_Y*2));
	GUI_SetColor(GUI_BLACK);
}


/*******************************************************************************
* Function Name: void show_text_modes(void)
********************************************************************************/
static void Show_Graph_Window(void)
{
	Graph_func.hGraph = GRAPH_CreateEx(0, 0, LCD_GetXSize(), LCD_GetYSize()-20,
					WM_HBKWIN, WM_CF_SHOW|WM_CF_MEMDEV_ON_REDRAW, GRAPH_CF_GRID_FIXED_X, GUI_ID_GRAPH0);

	// Window1 Create & Position Set
	GRAPH_SetGridVis(Graph_func.hGraph, 1);     // Enable grid
	GRAPH_SetGridDistX(Graph_func.hGraph, 10);  // Set X size of grid to 10 pixels
	GRAPH_SetGridDistY(Graph_func.hGraph, 10);  // Set Y size of grid to 10 pixels
	GRAPH_SetGridOffY(Graph_func.hGraph, 0);   // Set offset of grid to match with the graph

	// Boarder Position Set
	GRAPH_SetBorder(Graph_func.hGraph, 30, 0, 0, 0);

	// DATA Graph Line Set
	// TEMP
	Graph_func.hData[0] = GRAPH_DATA_YT_Create(GUI_RED, LCD_GetXSize(), NULL, 0);
	GRAPH_DATA_YT_SetAlign(Graph_func.hData[0], GRAPH_ALIGN_LEFT);
	GRAPH_SetVSizeX(Graph_func.hData[0], LCD_GetXSize());
	GRAPH_DATA_YT_SetOffY(Graph_func.hData[0], 50);
	GRAPH_AttachData(Graph_func.hGraph, Graph_func.hData[0]);

	// HUMI
	Graph_func.hData[1] = GRAPH_DATA_YT_Create(GUI_GREEN, LCD_GetXSize(), NULL, 0);
	GRAPH_DATA_YT_SetAlign(Graph_func.hData[1], GRAPH_ALIGN_LEFT);
	GRAPH_SetVSizeX(Graph_func.hData[1], LCD_GetXSize());
	GRAPH_DATA_YT_SetOffY(Graph_func.hData[1], 50);
	GRAPH_AttachData(Graph_func.hGraph, Graph_func.hData[1]);

	// LIGHT
	Graph_func.hData[2] = GRAPH_DATA_YT_Create(GUI_MAGENTA, LCD_GetXSize(), NULL, 0);
	GRAPH_DATA_YT_SetAlign(Graph_func.hData[2], GRAPH_ALIGN_LEFT);
	GRAPH_SetVSizeX(Graph_func.hData[2], LCD_GetXSize());
	GRAPH_DATA_YT_SetOffY(Graph_func.hData[2], 50);
	GRAPH_AttachData(Graph_func.hGraph, Graph_func.hData[2]);

	// MOTION
	Graph_func.hData[3] = GRAPH_DATA_YT_Create(GUI_YELLOW, LCD_GetXSize(), NULL, 0);
	GRAPH_DATA_YT_SetAlign(Graph_func.hData[3], GRAPH_ALIGN_LEFT);
	GRAPH_SetVSizeX(Graph_func.hData[3], LCD_GetXSize());
	GRAPH_DATA_YT_SetOffY(Graph_func.hData[3], 50);
	GRAPH_AttachData(Graph_func.hGraph, Graph_func.hData[3]);

	// Boarder Variable Set
	Graph_func.hScale = GRAPH_SCALE_Create(15, GUI_TA_HCENTER | GUI_TA_VCENTER, GRAPH_SCALE_CF_VERTICAL, 1);
	GRAPH_SCALE_SetNumDecs(Graph_func.hScale, 0);    // No decimals are shown on the scale
	GRAPH_SCALE_SetFactor(Graph_func.hScale, 1);     // Factor of the numbers on the scale
	GRAPH_SCALE_SetTickDist(Graph_func.hScale, 20);  // Distance in pixels between each number on the scale
	GRAPH_SCALE_SetOff(Graph_func.hScale, 50);      // Set y-offset of the scale to show negative numbers
	GRAPH_AttachScale(Graph_func.hGraph, Graph_func.hScale);

	// Text
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_RED);
	GUI_DispStringAt("TEMP", LCD_GetXSize()-LCD_GetXSize()+40, LCD_GetYSize()-20);

	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_GREEN);
	GUI_DispStringAt("HUMI", LCD_GetXSize()-LCD_GetXSize()+100, LCD_GetYSize()-20);

	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_MAGENTA);
	GUI_DispStringAt("LIGHT", LCD_GetXSize()-LCD_GetXSize()+170, LCD_GetYSize()-20);

	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_YELLOW);
	GUI_DispStringAt("MOTION", LCD_GetXSize()-LCD_GetXSize()+250, LCD_GetYSize()-20);

    GUI_Delay(1);
    GRAPH_DATA_YT_AddValue(Graph_func.hData[0], (I16)0);
    GRAPH_DATA_YT_AddValue(Graph_func.hData[1], (I16)0);
    GRAPH_DATA_YT_AddValue(Graph_func.hData[2], (I16)0);
    GRAPH_DATA_YT_AddValue(Graph_func.hData[3], (I16)0);
    //WM_MULTIBUF_Enable(0);
}

static void Show_Text_Window_Update(void)
{
	uint8_t cnt=0;
    // Temparature

    GUI_SetTextAlign(GUI_TA_HCENTER);
    GUI_SetTextMode(GUI_TEXTMODE_REV);
    GUI_SetColor(GUI_WHITE);
    GUI_DispDecAt( (sys_Data.d_Sens.temp.val[0])|((sys_Data.d_Sens.temp.val[1])<<8),
    			TEMP_STR_POS_X+(GUI_STR_VAL_OFFSET_X+10), TEMP_STR_POS_Y,3);
    GUI_SetColor(GUI_BLACK);

    // Gyro
    //for(cnt=0;cnt<3;cnt++)
    {
        GUI_SetTextAlign(GUI_TA_HCENTER);
    	GUI_SetTextMode(GUI_TEXTMODE_REV);
    	GUI_SetColor(GUI_WHITE);
    	GUI_DispDecAt(sys_Data.d_Sens.light.val[cnt], GYRO_STR_POS_X+(GUI_STR_VAL_OFFSET_X+10),
    			GYRO_STR_POS_Y+(GUI_STR_VAL_OFFSET_Y*1),3);
    	GUI_SetColor(GUI_BLACK);
    }

	// Light
	GUI_SetTextAlign(GUI_TA_HCENTER);
	GUI_SetTextMode(GUI_TEXTMODE_REV);
	GUI_SetColor(GUI_WHITE);
	GUI_DispDecAt(sys_Data.d_Sens.humi.val[0]
			, HUMI_STR_POS_X+GUI_STR_VAL_OFFSET_X, HUMI_STR_POS_Y,3);
	GUI_SetColor(GUI_BLACK);

    // Motion
    for(cnt=0;cnt<3;cnt++)
    {
        GUI_SetTextAlign(GUI_TA_HCENTER);
    	GUI_SetTextMode(GUI_TEXTMODE_REV);
    	GUI_SetColor(GUI_WHITE);
    	GUI_DispDecAt(sys_Data.d_Sens.accel.val[cnt],  ACCEL_STR_POS_X+GUI_STR_VAL_OFFSET_X,
    			 ACCEL_STR_POS_Y+(GUI_STR_VAL_OFFSET_Y*cnt),3);
    	GUI_SetColor(GUI_BLACK);
    }
}
/*******************************************************************************
* Function Name: void eInk_task(void *arg)
********************************************************************************/


void LCD_Queue(LCD_Command_t LCD_Req_Q)
{
	xQueueSend(TFT_LCD_CommandQ,&LCD_Req_Q,portMAX_DELAY);
}

static void Emwin_Clear_Window(void)
{
    //GUI_SetFont(GUI_FONT_16B_1);
    //GUI_SetColor(GUI_BLACK);
    GUI_SetBkColor(GUI_BLACK);
    GUI_Clear();

    // GUI
	GRAPH_DATA_YT_Clear(Graph_func.hData[0]);
	GRAPH_DATA_YT_Clear(Graph_func.hData[1]);
	GRAPH_DATA_YT_Clear(Graph_func.hData[2]);
	GRAPH_DATA_YT_Clear(Graph_func.hData[3]);
	Graph_Limit_count=0;

    WM_Deactivate();
}

static void Show_Graph_Window_Update(void)
{
	uint16_t temp=0;
	uint16_t humi=0;
	uint16_t light=0;
	uint16_t motion=0;

	temp=sys_Data.d_Sens.temp.val[0];
    humi=sys_Data.d_Sens.humi.val[0];
	light=sys_Data.d_Sens.light.val[0];
	motion=sys_Data.d_Sens.accel.val[0];

	GUI_Delay(1);
	//WM_MULTIBUF_Enable(1);
	GRAPH_DATA_YT_AddValue(Graph_func.hData[0], (I16)temp);
	GRAPH_DATA_YT_AddValue(Graph_func.hData[1], (I16)humi);
	GRAPH_DATA_YT_AddValue(Graph_func.hData[2], (I16)light);
	GRAPH_DATA_YT_AddValue(Graph_func.hData[3], (I16)motion);

	if(Graph_Limit_count>290)
	{
		GUI_Delay(1);
		GRAPH_DATA_YT_Clear(Graph_func.hData[0]);
		GRAPH_DATA_YT_Clear(Graph_func.hData[1]);
		GRAPH_DATA_YT_Clear(Graph_func.hData[2]);
		GRAPH_DATA_YT_Clear(Graph_func.hData[3]);
		Graph_Limit_count=0;
	}
	else
		Graph_Limit_count++;

	//WM_MULTIBUF_Enable(0);
}


/*******************************************************************************
* Function Name: void eInk_task(void *arg)
********************************************************************************/
void Emwin_task(void *arg)
{
    BaseType_t rtosApiResult;
	LCD_Command_t TFT_command;
    Emwin_Driver_Setup();
	TFT_LCD_CommandQ=xQueueCreate(1u, sizeof(LCD_Command_t));

	Show_startup_screen();
    for(;;)
    {
		rtosApiResult = xQueueReceive(TFT_LCD_CommandQ, &TFT_command, portMAX_DELAY);

		if(rtosApiResult==pdTRUE)
		{
			switch(TFT_command)
			{
			case DISP_IDLE:

				break;

			case DISP_LOGO:
				Emwin_Clear_Window();
				Show_Logo();
				break;

			case DISP_TEXT:
				Emwin_Clear_Window();
				Show_Text_Window();
				break;

			case DISP_TEXT_UPDATE:
				Show_Text_Window_Update();
				break;

			case DISP_GRAPH:
				Emwin_Clear_Window();
				Show_Graph_Window();
				WM_Activate();
				break;

			case DISP_GRAPH_UPDATE:
				Show_Graph_Window_Update();
				break;

			default:

				break;
			}
		}
    }
}
