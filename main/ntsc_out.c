/* Copyright (c) 2020, Peter Barrett
 **
 ** Permission to use, copy, modify, and/or distribute this software for
 ** any purpose with or without fee is hereby granted, provided that the
 ** above copyright notice and this permission notice appear in all copies.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 ** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 ** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
 ** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 ** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 ** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 ** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 ** SOFTWARE.
 */

#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32/rom/lldesc.h"
#include "driver/dac.h"
#include "driver/i2s.h"

#include "ntsc_out.h"

uint8_t **_lines;

//====================================================================================================
//====================================================================================================
//
// low level HW setup of DAC/DMA/APLL/PWM
//

static lldesc_t _dma_desc[4] = { 0 };
static intr_handle_t _isr_handle;

static void IRAM_ATTR video_isr(volatile void *buf);

// simple isr
static void IRAM_ATTR i2s_intr_handler_video(void *arg) {
	if (I2S0.int_st.out_eof)
		video_isr(((lldesc_t*) I2S0.out_eof_des_addr)->buf); // get the next line of video
	I2S0.int_clr.val = I2S0.int_st.val;                   // reset the interrupt
}

static esp_err_t start_dma(int line_width) {
	int ch = 1;

	periph_module_enable(PERIPH_I2S0_MODULE);

	printf("Allocating interrupt on core %d\n", xPortGetCoreID());

	// setup interrupt
	if (esp_intr_alloc(ETS_I2S0_INTR_SOURCE,
	ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, i2s_intr_handler_video, 0,
			&_isr_handle) != ESP_OK)
		return -1;

	// reset conf
	I2S0.conf.val = 1;
	I2S0.conf.val = 0;
	I2S0.conf.tx_right_first = 1;
	I2S0.conf.tx_mono = (ch == 2 ? 0 : 1);

	I2S0.conf2.lcd_en = 1;
	I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
	I2S0.sample_rate_conf.tx_bits_mod = 16;
	I2S0.conf_chan.tx_chan_mod = (ch == 2) ? 0 : 1;

	// Create TX DMA buffers
	for (int i = 0; i < 2; i++) {
		int n = line_width * 2 * ch;
		if (n >= 4092) {
			printf("DMA chunk too big: %d\n", n);
			return -1;
		}
		_dma_desc[i].buf = (uint8_t*) heap_caps_calloc(1, n, MALLOC_CAP_DMA);
		if (!_dma_desc[i].buf)
			return -1;

		_dma_desc[i].owner = 1;
		_dma_desc[i].eof = 1;
		_dma_desc[i].length = n;
		_dma_desc[i].size = n;
		_dma_desc[i].empty = (uint32_t) (i == 1 ? _dma_desc : _dma_desc + 1);
	}
	I2S0.out_link.addr = (uint32_t) _dma_desc;

	//  Setup up the apll: See ref 3.2.7 Audio PLL
	//  f_xtal = (int)rtc_clk_xtal_freq_get() * 1000000;
	//  f_out = xtal_freq * (4 + sdm2 + sdm1/256 + sdm0/65536); // 250 < f_out < 500
	//  apll_freq = f_out/((o_div + 2) * 2)
	//  operating range of the f_out is 250 MHz ~ 500 MHz
	//  operating range of the apll_freq is 16 ~ 128 MHz.
	//  select sdm0,sdm1,sdm2 to produce nice multiples of colorburst frequencies

	//  see calc_freq() for math: (4+a)*10/((2 + b)*2) mhz
	//  up to 20mhz seems to work ok:
	// rtc_clk_apll_enable(1,0x00,0x00,0x4,0);   // 20mhz for fancy DDS

	// 14.3181818182 4x NTSC (14.3181864421mhz)
	rtc_clk_apll_enable(1, 0x46, 0x97, 0x4, 1);

	I2S0.clkm_conf.clkm_div_num = 1;      // I2S clock divider's integral value.
	I2S0.clkm_conf.clkm_div_b = 0; // Fractional clock divider's numerator value.
	I2S0.clkm_conf.clkm_div_a = 1; // Fractional clock divider's denominator value
	I2S0.sample_rate_conf.tx_bck_div_num = 1;
	I2S0.clkm_conf.clka_en = 1;              // Set this bit to enable clk_apll.
	I2S0.fifo_conf.tx_fifo_mod = (ch == 2) ? 0 : 1; // 32-bit dual or 16-bit single channel data

	dac_output_enable(DAC_CHANNEL_1);           // DAC, video on GPIO25
	dac_i2s_enable();                           // start DAC!

	I2S0.conf.tx_start = 1;                     // start DMA!
	I2S0.int_clr.val = 0xFFFFFFFF;
	I2S0.int_ena.out_eof = 1;
	I2S0.out_link.start = 1;
	return esp_intr_enable(_isr_handle);        // start interruprs!
}

static void video_init_hw(int line_width) {
	// setup apll 4x NTSC colorburst rate
	start_dma(line_width);
}

// NTSC OUT PART
static TaskHandle_t _repaint_task = NULL;

static volatile int _line_counter = 0;
static volatile int _frame_counter = 0;

static int _line_width;
static const uint32_t *_palette;

static bool _debug = false;
static float _sample_rate;

static int _hsync;
static int _half_line;
static int _hsync_long;
static int _hsync_short;
static int _burst_start;
static int _active_start;

static int usec(float us) {
	uint32_t r = (uint32_t) (us * _sample_rate);
	return r - (r % 4);
}

#define NTSC_FREQUENCY (315000000.0/88)

void video_init(const uint32_t *palette, int ntsc, TaskHandle_t repaint_task) {
	_palette = palette;
	_repaint_task = repaint_task;

	_sample_rate = NTSC_FREQUENCY / 1000000 * 4; // 14,3181818182 MHz
	_line_width = usec(63.7); // 228, should be usec(63.555)
	_hsync_short = usec(2.3);
	_hsync_long = usec(27.1);
	_hsync = usec(4.7);
	_half_line = (_line_width / 2) - (_line_width / 2) % 4;
	_burst_start = usec(4.7 + 0.6); // breeze away 0.6us
	_active_start = usec(4.7 + 4.7) + 20; // offset of 5 color clock

	video_init_hw(_line_width);    // init the hardware
}

#ifdef PERF
#define BEGIN_TIMING()  uint32_t t = cpu_ticks()
#define END_TIMING() t = cpu_ticks() - t; _blit_ticks_min = min(_blit_ticks_min,t); _blit_ticks_max = max(_blit_ticks_max,t);
#define ISR_BEGIN() uint32_t t = cpu_ticks()
#define ISR_END() t = cpu_ticks() - t;_isr_us += (t+120)/240;
uint32_t _blit_ticks_min = 0;
uint32_t _blit_ticks_max = 0;
uint32_t _isr_us = 0;
#else
#define BEGIN_TIMING()
#define END_TIMING()
#define ISR_BEGIN()
#define ISR_END()
#endif

// draw a line in NTSC
static void IRAM_ATTR image( uint16_t *i2s_buffer, uint8_t *src) {
	uint32_t *d = (uint32_t*) i2s_buffer;
	const uint32_t *p = _palette;

	BEGIN_TIMING();

	// 192 color clocks, 384 pixels
	for (int i = 0; i < 362; i += 4) {
		uint32_t c = *((uint32_t*) src); // screen may be in 32 bit mem
		d[0] = p[(uint8_t) c];
		d[1] = p[(uint8_t) (c >> 8)] << 8;
		d[2] = p[(uint8_t) (c >> 16)];
		d[3] = p[(uint8_t) (c >> 24)] << 8;
		d += 4;
		src += 4;
	}

	END_TIMING();
}

static void IRAM_ATTR burst(uint16_t *line) {
	for (int i = 0; i < 4 * 9; i += 4) {
		line[i + 1] = BLANKING_LEVEL;
		line[i + 0] = BLANKING_LEVEL + BLANKING_LEVEL / 2;
		line[i + 3] = BLANKING_LEVEL;
		line[i + 2] = BLANKING_LEVEL - BLANKING_LEVEL / 2;
	}
}

static void IRAM_ATTR fill(uint16_t *line, int16_t level, int width) {
	for (int i = 0; i < width; i++)
		line[i] = level;
}

// Workhorse ISR handles audio and video updates
static void IRAM_ATTR video_isr(volatile void *vbuf) {
	if (!_lines)
		return;

	ISR_BEGIN();

	int i = _line_counter++;
	uint16_t *line = (uint16_t*) vbuf;

	fill(line, BLANKING_LEVEL, _line_width);
	// First field
	if (_debug) {
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
	} else 	if (i < 3) {
		fill(line, SYNC_LEVEL, _hsync_short);
		fill(line + _half_line, SYNC_LEVEL, _hsync_short);
	} else if (i < 6) {
		fill(line, SYNC_LEVEL, _hsync_long);
		fill(line + _half_line, SYNC_LEVEL, _hsync_long);
	} else if (i < 9) {
		fill(line, SYNC_LEVEL, _hsync_short);
		fill(line + _half_line, SYNC_LEVEL, _hsync_short);
	} else if (i < 29) {
		// the image should start @19, but my cheap screen shows from 29.
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
	} else if (i < 254) {
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
		// the image should start @19, but my cheap screen shows from 29.
		image(line + _active_start, _lines[i - 29]);
	} else if (i < 262) {
		// the image should finish @263, but my cheap screen shows only 225 lines.
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
	} else if (i < 263) {
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
		// Second field (264 = 1)
		fill(line + _half_line, BLANKING_LEVEL, _half_line);
		fill(line + _half_line, SYNC_LEVEL, _hsync_short);
	} else if (i < 265) {
		fill(line, SYNC_LEVEL, _hsync_short);
		fill(line + _half_line, SYNC_LEVEL, _hsync_short);
	} else if (i < 266) {
		fill(line, SYNC_LEVEL, _hsync_short);
		fill(line + _half_line, SYNC_LEVEL, _hsync_long);
	} else if (i < 268) {
		fill(line, SYNC_LEVEL, _hsync_long);
		fill(line + _half_line, SYNC_LEVEL, _hsync_long);
	} else if (i < 269) {
		fill(line, SYNC_LEVEL, _hsync_long);
		fill(line + _half_line, SYNC_LEVEL, _hsync_short);
	} else if (i < 271) {
		fill(line, SYNC_LEVEL, _hsync_short);
		fill(line + _half_line, SYNC_LEVEL, _hsync_short);
	} else if (i < 272) {
		fill(line, SYNC_LEVEL, _hsync_short);
	} else if (i < 292) { // the image should start @282, but my cheap screen shows from 292.
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
	} else if (i < 517) { // the image should finish @525, but my cheap screen shows only 225 lines.
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
		// the image should start @282, but my cheap screen shows from 292.
		image(line + _active_start, _lines[i - 292]);
	} else if (i < 525) {
		// the image should finish @525, but my cheap screen shows only 225 lines.
		fill(line, SYNC_LEVEL, _hsync);
		burst(line + _burst_start);
		if (i == 524) {
			if(_repaint_task != NULL) {
				xTaskNotifyFromISR(_repaint_task, 0, eNoAction, NULL);
			}
			_line_counter = 0;                      // frame is done
			_frame_counter++;
		}
	}
	ISR_END();
}

