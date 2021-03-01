/*
 * dashboard.c
 *
 *  Created on: Feb 27, 2021
 *      Author: eric
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/rtc.h"

#include "../video_out/video_out.h"

#define ULONG unsigned int
static void* MALLOC32(int x, const char *label) {
	printf("MALLOC32 %d free, %d biggest, allocating %s: %d\n",
			heap_caps_get_free_size(MALLOC_CAP_32BIT),
			heap_caps_get_largest_free_block(MALLOC_CAP_32BIT), label, x);
	void *r = heap_caps_malloc(x, MALLOC_CAP_32BIT);
	if (!r) {
		printf("MALLOC32 FAILED allocation of %s: %d\n", label, x);
		esp_restart();
	} else
		printf("MALLOC32 allocation of %s: %d @ %08X\n", label, x,
				(unsigned int) r);
	return r;
}

#define dashboard_screen_WIDTH  384
#define dashboard_screen_HEIGHT 240 // Works with 288

static ULONG *dashboard_screen_0;
static uint8_t **dashboard_screen_0_lines;
static ULONG *dashboard_screen_1;
static uint8_t **dashboard_screen_1_lines;

static ULONG *dashboard_screen_current;
static uint8_t **dashboard_screen_current_lines;

void dashboard_init(void) {
	// 32 bit access video buffer
	dashboard_screen_0 = (ULONG*) MALLOC32(dashboard_screen_WIDTH * dashboard_screen_HEIGHT, "video buffer 1");
	dashboard_screen_0_lines = (uint8_t**) MALLOC32(dashboard_screen_HEIGHT * sizeof(uint8_t*), "video buffer 1 by line_index");
	dashboard_screen_1 = (ULONG*) MALLOC32(dashboard_screen_WIDTH * dashboard_screen_HEIGHT, "video buffer 2");
	dashboard_screen_1_lines = (uint8_t**) MALLOC32(dashboard_screen_HEIGHT * sizeof(uint8_t*), "video buffer 2 by line_index");
	// 8-bit pointer on the video buffer
	const uint8_t *s0 = (uint8_t*) dashboard_screen_0;
	const uint8_t *s1 = (uint8_t*) dashboard_screen_1;
	for (int y = 0; y < dashboard_screen_HEIGHT; y++) {
		dashboard_screen_0_lines[y] = (uint8_t*) s0;
		s0 += dashboard_screen_WIDTH;
		dashboard_screen_1_lines[y] = (uint8_t*) s1;
		s1 += dashboard_screen_WIDTH;
	}

	dashboard_screen_current = dashboard_screen_0;
	dashboard_screen_current_lines = dashboard_screen_0_lines;
}

// 32 bit aligned access
static inline uint8_t getp(const uint8_t *src) {
	size_t s = (size_t) src;
	return *((uint32_t*) (s & ~3)) >> ((s & 3) << 3);
}

static inline void setp(uint8_t *dst, uint8_t p) {
	size_t s = (size_t) dst;
	size_t shift = (s & 3) << 3;
	uint32_t *d = (uint32_t*) (s & ~3);
	d[0] = (d[0] & ~(0xFF << shift)) | (p << shift);
}

static void clear_screen() {
	int i = dashboard_screen_WIDTH * dashboard_screen_HEIGHT / 4;
	while (i--)
		dashboard_screen_current[i] = 0;
}

// Used to find margins of a given screen
void test_pattern() {
	// Plain white
	for (int y = 0; y < dashboard_screen_HEIGHT; y++) {
		uint8_t *line = (uint8_t*) dashboard_screen_current_lines[y];
		for (int x = 0; x < dashboard_screen_WIDTH; x++) {
			setp(line + x, 15);
		}
	}

	int first_cell_number = 0;
	int cell_number = 0;
	int cell_width = dashboard_screen_WIDTH / 16;
	int cell_height = dashboard_screen_HEIGHT / 16;
	for (int y = 0; y < dashboard_screen_HEIGHT; y++) {
		uint8_t *line = (uint8_t*) dashboard_screen_current_lines[y];
		if (y % cell_height) {
			cell_number = first_cell_number;
			for (int x = 0; x < dashboard_screen_WIDTH;
					x++) {
				if (x % cell_width) {
					setp(line + x, cell_number % 256);
				} else {
					setp(line + x, 0);
					setp(line + x - 1, 0);
					cell_number++;
				}
			}
		} else {
			for (int x = 0; x < dashboard_screen_WIDTH; x++) {
				setp(line + x, 0);
			}
			first_cell_number = cell_number;
		}
	}
}

// Used to find margins of a given screen
void test_pattern2() {
	// Plain white
	clear_screen(15);

	int margin_top = 13;
	int margin_left = 9;
	int margin_bottom = 9;
	int margin_right = 19;
	// 356x218 after margins
	int first_cell_number = 0;
	int cell_number = 0;
	int cell_width = 1
			+ (dashboard_screen_WIDTH - margin_left - margin_right) / 16;
	int cell_height = (dashboard_screen_HEIGHT - margin_top - margin_bottom)
			/ 16;
	for (int y = margin_top; y < dashboard_screen_HEIGHT - margin_bottom; y++) {
		uint8_t *line = (uint8_t*) dashboard_screen_current_lines[y];
		if ((y - margin_top) % cell_height) {
			cell_number = first_cell_number;
			for (int x = margin_left; x < dashboard_screen_WIDTH - margin_right;
					x++) {
				if ((x - margin_left) % cell_width) {
					setp(line + x, cell_number % 256);
				} else {
					setp(line + x, 0);
					setp(line + x - 1, 0);
					cell_number++;
				}
			}
		} else {
			for (int x = margin_left; x < dashboard_screen_WIDTH - margin_right;
					x++) {
				setp(line + x, 0);
			}
			first_cell_number = cell_number;
		}
	}
}

static void dashboard_repaint(void) {
	clear_screen();
	test_pattern();
}

static uint8_t** dashboard_video_buffer() {
	if(dashboard_screen_current == dashboard_screen_0) {
		dashboard_screen_current = dashboard_screen_1;
		dashboard_screen_current_lines = dashboard_screen_1_lines;
		return dashboard_screen_0_lines;
	}
	dashboard_screen_current = dashboard_screen_0;
	dashboard_screen_current_lines = dashboard_screen_0_lines;
	return dashboard_screen_1_lines;
}

static void dashboard_task(void *arg) {
	rtc_cpu_freq_config_t *out_config = malloc(sizeof(rtc_cpu_freq_config_t));
	rtc_clk_cpu_freq_get_config(out_config);

	printf("dashboard_task running on core %d at %d MHz\n", xPortGetCoreID(),
			out_config->freq_mhz);

	free(out_config);

	_lines = dashboard_video_buffer();
	for (;;) {
		dashboard_repaint();
		if(xTaskNotifyWait(0x00, ULONG_MAX, NULL, 100)) {
			_lines = dashboard_video_buffer();
		} else {
			printf("Timeout %d\n", xthal_get_ccount());
		}
	}
}


TaskHandle_t dashboard_create_task(void) {
	TaskHandle_t dashboard_task_handle = NULL;
	BaseType_t coreId = 1;
	xTaskCreatePinnedToCore(dashboard_task, "dashboard_task", 5 * 1024, NULL, 0, &dashboard_task_handle, coreId); // 5k word stack, start on core 0
	return dashboard_task_handle;
}
