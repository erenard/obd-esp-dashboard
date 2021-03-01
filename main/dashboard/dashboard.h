/*
 * dashboard.h
 *
 *  Created on: Feb 27, 2021
 *      Author: eric
 */

#ifndef MAIN_DASHBOARD_H_
#define MAIN_DASHBOARD_H_

uint8_t** dashboard_init(void);
TaskHandle_t * dashboard_create_task(void);

// uint8_t dashboard_get_pixel(const uint8_t* position);
// void dashboard_set_pixel(uint8_t* position, uint8_t pixel);

#endif /* MAIN_DASHBOARD_H_ */
