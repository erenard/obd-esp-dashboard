/*
 * framebuffer.c
 *
 *  Created on: Feb 27, 2021
 *      Author: eric
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/rtc.h"

#include "framebuffer.h"

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

static ULONG *framebuffer_screen_0;
static uint8_t **framebuffer_screen_0_lines;
static ULONG *framebuffer_screen_1;
static uint8_t **framebuffer_screen_1_lines;

static ULONG *framebuffer_screen_front;
static uint8_t **framebuffer_screen_front_lines;

static ULONG *framebuffer_screen_back;
static uint8_t **framebuffer_screen_back_lines;

/*
 * PUBLIC
 */

void framebuffer_init(void) {
	// 32 bit access video buffer
	framebuffer_screen_0 = (ULONG*) MALLOC32(framebuffer_screen_WIDTH * framebuffer_screen_HEIGHT, "video buffer 1");
	framebuffer_screen_0_lines = (uint8_t**) MALLOC32(framebuffer_screen_HEIGHT * sizeof(uint8_t*), "video buffer 1 by line_index");
	framebuffer_screen_1 = (ULONG*) MALLOC32(framebuffer_screen_WIDTH * framebuffer_screen_HEIGHT, "video buffer 2");
	framebuffer_screen_1_lines = (uint8_t**) MALLOC32(framebuffer_screen_HEIGHT * sizeof(uint8_t*), "video buffer 2 by line_index");
	// 8-bit pointer on the video buffer
	const uint8_t *s0 = (uint8_t*) framebuffer_screen_0;
	const uint8_t *s1 = (uint8_t*) framebuffer_screen_1;
	for (int y = 0; y < framebuffer_screen_HEIGHT; y++) {
		framebuffer_screen_0_lines[y] = (uint8_t*) s0;
		s0 += framebuffer_screen_WIDTH;
		framebuffer_screen_1_lines[y] = (uint8_t*) s1;
		s1 += framebuffer_screen_WIDTH;
	}

	framebuffer_screen_front = framebuffer_screen_0;
	framebuffer_screen_front_lines = framebuffer_screen_0_lines;

	framebuffer_screen_back = framebuffer_screen_1;
	framebuffer_screen_back_lines = framebuffer_screen_1_lines;
}

uint8_t** framebuffer_get_front() {
	return framebuffer_screen_front_lines;
}

uint8_t** framebuffer_get_back() {
	return framebuffer_screen_back_lines;
}

void framebuffer_swap() {
	if(framebuffer_screen_front == framebuffer_screen_0) {
		framebuffer_screen_front = framebuffer_screen_1;
		framebuffer_screen_front_lines = framebuffer_screen_1_lines;
		framebuffer_screen_back = framebuffer_screen_0;
		framebuffer_screen_back_lines = framebuffer_screen_0_lines;
	} else {
		framebuffer_screen_front = framebuffer_screen_0;
		framebuffer_screen_front_lines = framebuffer_screen_0_lines;
		framebuffer_screen_back = framebuffer_screen_1;
		framebuffer_screen_back_lines = framebuffer_screen_1_lines;
	}
}

void framebuffer_clear() {
	int i = framebuffer_screen_WIDTH * framebuffer_screen_HEIGHT / 4;
	while (i--)
		framebuffer_screen_back[i] = 0;
}

// 32 bit aligned access
inline uint8_t framebuffer_get_pixel(const uint8_t *src) {
	size_t s = (size_t) src;
	return *((uint32_t*) (s & ~3)) >> ((s & 3) << 3);
}

inline void framebuffer_set_pixel(uint8_t *dst, uint8_t p) {
	size_t s = (size_t) dst;
	size_t shift = (s & 3) << 3;
	uint32_t *d = (uint32_t*) (s & ~3);
	d[0] = (d[0] & ~(0xFF << shift)) | (p << shift);
}
