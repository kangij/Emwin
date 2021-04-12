/*
 * TFT_LCD.h
 *
 *  Created on: 2021. 4. 5.
 *      Author: DBC116
 */

#ifndef SOURCE_TFT_LCD_H_
#define SOURCE_TFT_LCD_H_


typedef enum _LCD_Command_t
{
	DISP_IDLE,
	DISP_LOGO,
	DISP_TEXT,
	DISP_TEXT_UPDATE,
	DISP_GRAPH,
	DISP_GRAPH_UPDATE
}LCD_Command_t;

void Emwin_task(void *arg);
void LCD_Queue(LCD_Command_t LCD_Req_Q);

#endif /* SOURCE_TFT_LCD_H_ */
