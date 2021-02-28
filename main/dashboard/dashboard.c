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

// swizzed ntsc palette in RAM
const uint32_t dashboard_4_phase_ntsc[256] = { 0x18181818, 0x19191919,
		0x1B1B1B1B, 0x1C1C1C1C, 0x1E1E1E1E, 0x1F1F1F1F, 0x21212121, 0x22222222,
		0x24242424, 0x25252525, 0x27272727, 0x28282828, 0x2A2A2A2A, 0x2B2B2B2B,
		0x2D2D2D2D, 0x2E2E2E2E, 0x30303030, 0x32323232, 0x33333333, 0x35353535,
		0x36363636, 0x38383838, 0x39393939, 0x3B3B3B3B, 0x3C3C3C3C, 0x3E3E3E3E,
		0x3F3F3F3F, 0x41414141, 0x42424242, 0x44444444, 0x45454545, 0x47474747,
		0x18181818, 0x19191918, 0x1A1A1B19, 0x1B1C1D1A, 0x1C1D1F1A, 0x1D1F211B,
		0x1F20231C, 0x2022251C, 0x2123271D, 0x2225291E, 0x23262B1E, 0x25282D1F,
		0x26292F20, 0x272B3120, 0x282C3321, 0x292E3522, 0x2B2F3723, 0x2C313923,
		0x2D323B24, 0x2E343D25, 0x2F353F25, 0x31374126, 0x32384327, 0x333A4527,
		0x343B4728, 0x353D4929, 0x373E4B29, 0x38404D2A, 0x39414F2B, 0x3A43512B,
		0x3B44532C, 0x3D46552D, 0x18181818, 0x19181819, 0x1B18191A, 0x1D181A1B,
		0x1F181B1D, 0x21181C1E, 0x23181D1F, 0x25181D20, 0x27191E22, 0x29191F23,
		0x2B192024, 0x2D192126, 0x2F192227, 0x31192228, 0x33192329, 0x351A242B,
		0x371A252C, 0x391A262D, 0x3B1A272F, 0x3D1A2730, 0x3F1A2831, 0x411A2932,
		0x431A2A34, 0x451B2B35, 0x471B2C36, 0x491B2C38, 0x4B1B2D39, 0x4D1B2E3A,
		0x4F1B2F3B, 0x511B303D, 0x531C313E, 0x551C323F, 0x18181818, 0x19181918,
		0x1B181A18, 0x1D181C19, 0x1E181D19, 0x20181E1A, 0x2218201A, 0x2318211B,
		0x2519221B, 0x2719241C, 0x2819251C, 0x2A19261D, 0x2C19281D, 0x2D19291E,
		0x2F192A1E, 0x31192C1F, 0x321A2D1F, 0x341A2E1F, 0x361A3020, 0x371A3120,
		0x391A3221, 0x3B1A3421, 0x3D1A3522, 0x3E1A3622, 0x401B3823, 0x421B3923,
		0x431B3A24, 0x451B3C24, 0x471B3D25, 0x481B3E25, 0x4A1B4026, 0x4C1C4126,
		0x18181818, 0x17191819, 0x171A181A, 0x171C181B, 0x171D181C, 0x171F181D,
		0x1720191E, 0x1621191F, 0x16231920, 0x16241921, 0x16261922, 0x16271A23,
		0x16281A24, 0x162A1A25, 0x152B1A26, 0x152D1A27, 0x152E1B28, 0x152F1B2A,
		0x15311B2B, 0x15321B2C, 0x14341B2D, 0x14351B2E, 0x14361C2F, 0x14381C30,
		0x14391C31, 0x143B1C32, 0x143C1C33, 0x133D1D34, 0x133F1D35, 0x13401D36,
		0x13421D37, 0x13431D38, 0x18181818, 0x17191818, 0x161A1918, 0x161B1918,
		0x151D1A18, 0x151E1B18, 0x141F1B19, 0x14211C19, 0x13221D19, 0x13231D19,
		0x12251E19, 0x12261E1A, 0x11271F1A, 0x1129201A, 0x102A201A, 0x102B211A,
		0x0F2D221A, 0x0F2E221B, 0x0E2F231B, 0x0E31231B, 0x0D32241B, 0x0D33251B,
		0x0C35251C, 0x0C36261C, 0x0B37271C, 0x0A39271C, 0x0A3A281C, 0x093B281C,
		0x093D291D, 0x083E2A1D, 0x083F2A1D, 0x07412B1D, 0x18181818, 0x18181718,
		0x18181719, 0x1918161A, 0x1918161B, 0x1918151C, 0x1A18151D, 0x1A18141D,
		0x1A18141E, 0x1B18131F, 0x1B181320, 0x1B181221, 0x1C181222, 0x1C181122,
		0x1C181123, 0x1D181024, 0x1D181025, 0x1D181026, 0x1E180F27, 0x1E180F28,
		0x1E180E28, 0x1F180E29, 0x1F180D2A, 0x1F180D2B, 0x20180C2C, 0x20180C2D,
		0x20180B2D, 0x21190B2E, 0x21190A2F, 0x21190A30, 0x22190931, 0x22190932,
		0x18181818, 0x18181718, 0x18181719, 0x1918161A, 0x1918161B, 0x1918151C,
		0x1A18151D, 0x1A18141D, 0x1A18141E, 0x1B18131F, 0x1B181320, 0x1B181221,
		0x1C181222, 0x1C181122, 0x1C181123, 0x1D181024, 0x1D181025, 0x1D181026,
		0x1E180F27, 0x1E180F28, 0x1E180E28, 0x1F180E29, 0x1F180D2A, 0x1F180D2B,
		0x20180C2C, 0x20180C2D, 0x20180B2D, 0x21190B2E, 0x21190A2F, 0x21190A30,
		0x22190931, 0x22190932, };

const uint32_t* dashboard_generate_palette(int video_mode) {
	return dashboard_4_phase_ntsc;
}
