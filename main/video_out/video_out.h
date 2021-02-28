/*
 * video_out.h
 *
 *  Created on: Feb 27, 2021
 *      Author: eric
 */

#ifndef MAIN_VIDEO_OUT_VIDEO_OUT_H_
#define MAIN_VIDEO_OUT_VIDEO_OUT_H_

uint8_t ** _lines;
void video_init(const uint32_t* palette, int mode, const TaskHandle_t* dashboard_task);

#endif /* MAIN_VIDEO_OUT_VIDEO_OUT_H_ */
