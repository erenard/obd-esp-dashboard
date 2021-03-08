/*
 * graphics.c
 *
 *  Created on: Mar 6, 2021
 *      Author: eric
 */

#include "video_out.h"
#include "graphics.h"

#include "freertos/FreeRTOS.h"

#include "framebuffer.h"

static uint8_t** lines;

void graphics_init() {
	lines = framebuffer_get_back();
}

void graphics_draw_test_pattern() {
	// Used to find margins of a given screen
	// Plain white
	for (int y = 0; y < framebuffer_screen_HEIGHT; y++) {
		uint8_t *line = (uint8_t*) lines[y];
		for (int x = 0; x < framebuffer_screen_WIDTH; x++) {
			framebuffer_set_pixel(line + x, 15);
		}
	}

	int first_cell_number = 0;
	int cell_number = 0;
	int cell_width = framebuffer_screen_WIDTH / 16;
	int cell_height = framebuffer_screen_HEIGHT / 16;
	for (int y = 0; y < framebuffer_screen_HEIGHT; y++) {
		uint8_t *line = (uint8_t*) lines[y];
		if (y % cell_height) {
			cell_number = first_cell_number;
			for (int x = 0; x < framebuffer_screen_WIDTH; x++) {
				if (x % cell_width) {
					framebuffer_set_pixel(line + x, cell_number % 256);
				} else {
					framebuffer_set_pixel(line + x, 0);
					framebuffer_set_pixel(line + x - 1, 0);
					cell_number++;
				}
			}
		} else {
			for (int x = 0; x < framebuffer_screen_WIDTH; x++) {
				framebuffer_set_pixel(line + x, 0);
			}
			first_cell_number = cell_number;
		}
	}
}

// Used to find margins of a given screen
void test_pattern2() {
	// Plain white
	for (int y = 0; y < framebuffer_screen_HEIGHT; y++) {
		uint8_t *line = (uint8_t*) lines[y];
		for (int x = 0; x < framebuffer_screen_WIDTH; x++) {
			framebuffer_set_pixel(line + x, 15);
		}
	}

	int margin_top = 13;
	int margin_left = 9;
	int margin_bottom = 9;
	int margin_right = 19;
	// 356x218 after margins
	int first_cell_number = 0;
	int cell_number = 0;
	int cell_width = 1
			+ (framebuffer_screen_WIDTH - margin_left - margin_right) / 16;
	int cell_height = (framebuffer_screen_HEIGHT - margin_top - margin_bottom)
			/ 16;
	for (int y = margin_top; y < framebuffer_screen_HEIGHT - margin_bottom;
			y++) {
		uint8_t *line = (uint8_t*) lines[y];
		if ((y - margin_top) % cell_height) {
			cell_number = first_cell_number;
			for (int x = margin_left;
					x < framebuffer_screen_WIDTH - margin_right; x++) {
				if ((x - margin_left) % cell_width) {
					framebuffer_set_pixel(line + x, cell_number % 256);
				} else {
					framebuffer_set_pixel(line + x, 0);
					framebuffer_set_pixel(line + x - 1, 0);
					cell_number++;
				}
			}
		} else {
			for (int x = margin_left;
					x < framebuffer_screen_WIDTH - margin_right; x++) {
				framebuffer_set_pixel(line + x, 0);
			}
			first_cell_number = cell_number;
		}
	}
}

void graphics_flush() {
	if (xTaskNotifyWait(0x00, ULONG_MAX, NULL, 100)) {
		framebuffer_swap();
		lines = framebuffer_get_back();
		_lines = framebuffer_get_front();
	} else {
		printf("Timeout %d\n", xthal_get_ccount());
	}
}
