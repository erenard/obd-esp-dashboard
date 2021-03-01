/*
 * video_out.h
 *
 *  Created on: Feb 27, 2021
 *      Author: eric
 */

#ifndef MAIN_VIDEO_OUT_VIDEO_OUT_H_
#define MAIN_VIDEO_OUT_VIDEO_OUT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Color clock frequency is 315/88 (3.57954545455)
// DAC_MHZ is 315/11 or 8x color clock
// 455/2 color clocks per line, round up to maintain phase
// HSYNCH period is 44/315*455 or 63.55555..us
// Field period is 262*44/315*455 or 16651.5555us

#define IRE(_x)          ((uint32_t)(((_x)+40)*255/3.3/140) << 8)   // 3.3V DAC
#define SYNC_LEVEL       IRE(-40)
#define BLANKING_LEVEL   IRE(0)
#define BLACK_LEVEL      IRE(7.5)
#define GRAY_LEVEL       IRE(50)
#define WHITE_LEVEL      IRE(100)

uint8_t ** _lines;
void video_init(const uint32_t *palette, int ntsc, TaskHandle_t repaint_task);

#endif /* MAIN_VIDEO_OUT_VIDEO_OUT_H_ */
