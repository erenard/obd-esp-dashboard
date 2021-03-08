/*
 * application.c
 *
 *  Created on: Mar 6, 2021
 *      Author: eric
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc.h"

#include "graphics.h"

static void read_obd_data() {}

static void repaint_dashboard() {
	graphics_draw_full_grid();
}

static void application_task(void *arg) {
	rtc_cpu_freq_config_t *out_config = malloc(sizeof(rtc_cpu_freq_config_t));
	rtc_clk_cpu_freq_get_config(out_config);

	printf("application_task running on core %d at %d MHz\n", xPortGetCoreID(),
			out_config->freq_mhz);

	free(out_config);

	for (;;) {
		read_obd_data();
		repaint_dashboard();
		graphics_flush();
	}
}


TaskHandle_t application_create_task(void) {
	TaskHandle_t application_task_handle = NULL;
	BaseType_t coreId = 1;
	xTaskCreatePinnedToCore(application_task, "application_task", 5 * 1024, NULL, 0, &application_task_handle, coreId); // 5k word stack, start on core 0
	return application_task_handle;
}
