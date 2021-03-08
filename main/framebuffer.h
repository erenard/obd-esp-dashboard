/*
 * framebuffer.h
 *
 *  Created on: Feb 27, 2021
 *      Author: eric
 */

#ifndef MAIN_FRAMEBUFFER_H_
#define MAIN_FRAMEBUFFER_H_

#define framebuffer_screen_WIDTH  384
#define framebuffer_screen_HEIGHT 240 // Works with 288

void framebuffer_init();
uint8_t** framebuffer_get_front();
uint8_t** framebuffer_get_back();
void framebuffer_swap();

void framebuffer_clear();
uint8_t framebuffer_get_pixel(const uint8_t *position);
void framebuffer_set_pixel(uint8_t *position, uint8_t pixel);

#endif /* MAIN_DASHBOARD_H_ */
