#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "soc/rtc.h"

#include "constants.h"
#define MODE NTSC

#include "palette.h"
#include "video_out.h"
#include "framebuffer.h"
#include "graphics.h"
#include "application.h"

void setup_max_frequency() {
	uint32_t freq_mhz = 240;
	rtc_cpu_freq_config_t *out_config = malloc(sizeof(rtc_cpu_freq_config_t));
	rtc_clk_cpu_freq_get_config(out_config);
	if (rtc_clk_cpu_freq_mhz_to_config(freq_mhz, out_config)) {
		rtc_clk_cpu_freq_set_config(out_config);
	}
	free(out_config);
}

void app_main(void) {
	setup_max_frequency();
	framebuffer_init();
	_lines = framebuffer_get_front();
	graphics_init();
	TaskHandle_t application_task = application_create_task();
	const uint32_t *palette = palette_generate_palette(MODE);
	printf("video_init\n");
	video_init(palette, MODE, application_task);

	while (true) {
		// OBD Stuff
		// Dump some stats
//		perf();
		vTaskDelay(1);
	}
}
