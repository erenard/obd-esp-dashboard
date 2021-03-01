/*
 * palette.c
 *
 *  Created on: Mar 1, 2021
 *      Author: eric
 */
#include "palette.h"
#include "../video_out/video_out.h"
#include "constants.h"

#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"

uint32_t* palette_generate_palette(int mode) {
	if(mode == NTSC) {
	    // float color_inc = 28.6 * M_PI / 180.0;
	    #define RAD(_d) ((_d - 180) * M_PI / 180.0)         // (degree - 180) because the color burst phase is 180d

	    // https://mrob.com/pub/xapple2/colors.html
	    // Color name      phase ampl luma   -R- -G- -B-
	    // black    COLOR=0    0   0    0      0   0   0
	    // gray     COLOR=5    0   0   50    156 156 156
	    // grey     COLOR=10   0   0   50    156 156 156
	    // white    COLOR=15   0   0  100    255 255 255
	    // dk blue  COLOR=2    0  60   25     96  78 189
	    // lt blue  COLOR=7    0  60   75    208 195 255
	    // purple   COLOR=3   45 100   50    255  68 253
	    // purple   HCOLOR=2  45 100   50    255  68 253
	    // red      COLOR=1   90  60   25    227  30  96
	    // pink     COLOR=11  90  60   75    255 160 208
	    // orange   COLOR=9  135 100   50    255 106  60
	    // orange   HCOLOR=5 135 100   50    255 106  60
	    // brown    COLOR=8  180  60   25     96 114   3
	    // yellow   COLOR=13 180  60   75    208 221 141
	    // lt green COLOR=12 225 100   50     20 245  60
	    // green    HCOLOR=1 225 100   50     20 245  60
	    // dk green COLOR=4  270  60   25      0 163  96
	    // aqua     COLOR=14 270  60   75    114 255 208
	    // med blue COLOR=6  315 100   50     20 207 253
	    // blue     HCOLOR=6 315 100   50     20 207 253
	    // NTSC Hsync          0   0  -40      0   0   0
	    // NTSC black          0   0    7.5   41  41  41
	    // NTSC Gray75         0   0   77    212 212 212
	    // YIQ +Q             33 100   50    255  81 255
	    // NTSC magenta       61  82   36    255  40 181
	    // NTSC red          104  88   28    255  28  76
	    // YIQ +I            123 100   50    255  89  82
	    // NTSC yellow       167  62   69    221 198 121
	    // Color burst       180  40   0       0   4   0
	    // YIQ -Q            213 100   50     51 232  41
	    // NTSC green        241  82   48     12 234  97
	    // NTSC cyan         284  88   56     10 245 198
	    // YIQ -I            303 100   50      0 224 231
	    // NTSC blue         347  62   15     38  65 155

	    // https://www.eetimes.com/measuring-composite-video-signal-performance-requires-understanding-differential-gain-and-phase-part-1-of-2/
	    float color_phase_angles[8] = {
	        RAD(0),      // White
	        RAD(167.1),  // Yellow
	        RAD(283.5),  // Cyan
	        RAD(240.7),  // Green
	        RAD(60.7),   // Magenta
	        RAD(103.5),  // Red
	        RAD(347.1),  // Blue
	        RAD(347.1),  // Blue
	    };

	    float color_luminances[8] = {
	        IRE(100.0),  // White
	        IRE(89.5),   // Yellow
	        IRE(72.3),   // Cyan
	        IRE(61.8),   // Green
	        IRE(45.7),   // Magenta
	        IRE(32.2),   // Red
	        IRE(18.0),   // Blue
	        IRE(18.0),   // Blue
	    };

	    float color_amplitudes[8] = {
	        // "- 40" is needed to calculate relative to 0 IRE level
	        0,                        // White
	        IRE(130.8 - 48.1 - 40),   // Yellow
	        IRE(130.8 - 13.9 - 40),   // Cyan
	        IRE(116.4 - 7.2 - 40),    // Green
	        IRE(100.3 - (-8.9) - 40), // Magenta
	        IRE(93.6 - (-23.3) - 40), // Red
	        IRE(59.4 - (-23.3) - 40), // Blue
	        IRE(59.4 - (-23.3) - 40), // Blue
	    };

	    int i = 0;
	    uint32_t cc_colors[256];
	    for (int cr = 0; cr < 8; cr += 1) {
	        float angle = color_phase_angles[cr];
	        float luminance = color_luminances[cr];
	        float amplitude = color_amplitudes[cr];
	        for (int lm = 0; lm < 32; lm++) {
	            double y = BLANKING_LEVEL + (luminance - BLANKING_LEVEL) * (lm / 32.0);
	            int p[4];
	            for (int j = 0; j < 4; j++)
	                p[j] = y;
	            for (int j = 0; j < 4; j++)
	                p[j] += sin(angle + 2.0 * M_PI * j / 4.0) * (amplitude / 2.0) * (lm / 32.0);
	            uint32_t pi = 0;
	            for (int j = 0; j < 4; j++)
	                pi = (pi << 8) | p[j] >> 8;
	            cc_colors[i++] = pi;
	        }
	    }

	    uint32_t * esp_cc_colors = calloc(256, sizeof(uint32_t));
	    uint32_t esp_cc_color;
	    // swizzed pattern for esp32 is 0 2 1 3
	    printf("const uint32_t ntsc_4_phase_esp[256] = {\n");
	    for (int i = 0; i < 256; i++) {
	        esp_cc_color = cc_colors[i];
	        esp_cc_color = (esp_cc_color & 0xFF0000FF) | ((esp_cc_color << 8) & 0x00FF0000) | ((esp_cc_color >> 8) & 0x0000FF00);
	        esp_cc_colors[i] = esp_cc_color;
	        printf("0x%08X,",esp_cc_color);
	        if ((i & 7) == 7)
	            printf("\n");
	    }
	    printf("};\n");

	    return esp_cc_colors;
	} else {
		//
	    float color_phase_angles[16] = {
	        RAD(0),      // White
	        RAD(167.1),  // Yellow
	        RAD(283.5),  // Cyan
	        RAD(240.7),  // Green
	        RAD(60.7),   // Magenta
	        RAD(103.5),  // Red
	        RAD(347.1),  // Blue
	        RAD(347.1),  // Blue
	        RAD(0),      // White
	        RAD(167.1),  // Yellow
	        RAD(283.5),  // Cyan
	        RAD(240.7),  // Green
	        RAD(60.7),   // Magenta
	        RAD(103.5),  // Red
	        RAD(347.1),  // Blue
	        RAD(347.1),  // Blue
	    };
	    float color_luminances[16] = {
		        IRE(100.0),  // White
		        IRE(89.5),   // Yellow
		        IRE(72.3),   // Cyan
		        IRE(61.8),   // Green
		        IRE(45.7),   // Magenta
		        IRE(32.2),   // Red
		        IRE(18.0),   // Blue
		        IRE(18.0),   // Blue
		        IRE(100.0),  // White
		        IRE(89.5),   // Yellow
		        IRE(72.3),   // Cyan
		        IRE(61.8),   // Green
		        IRE(45.7),   // Magenta
		        IRE(32.2),   // Red
		        IRE(18.0),   // Blue
		        IRE(18.0),   // Blue
	    };

		//
	    uint32_t * _pal4 = calloc(512, sizeof(uint32_t));
	    uint32_t *even = _pal4;
	    uint32_t *odd = even + 256;

	    float chroma_scale = BLANKING_LEVEL/2/256;
	    chroma_scale *= 1.5;
	    float s,c,u,v;

	    for (int cr = 0; cr < 16; cr++) {
	        if (cr == 0) {
	            u = v = 0;
	        } else {
	            u = cos(color_phase_angles[16-cr]);
	            v = sin(color_phase_angles[16-cr]);
	        }
	        for (int lm = 0; lm < 16; lm++) {
	            uint32_t e = 0;
	            uint32_t o = 0;
	            for (int i = 0; i < 4; i++) {
	                float p = 2*M_PI*i/4;
	                s = sin(p)*chroma_scale;
	                c = cos(p)*chroma_scale;
	                uint8_t e0 = round(color_luminances[lm] + (s*u) + (c*v));
	                uint8_t o0 = round(color_luminances[lm] + (s*u) - (c*v));
	                e = (e << 8) | e0;
	                o = (o << 8) | o0;
	            }
	            *even++ = e;
	            *odd++ = o;
	        }
	    }

	    /*
	    printf("uint32_t dashboard_4_phase_pal[] = {\n");
	    for (int i = 0; i < 256*2; i++) {  // start with luminance map
	        printf("0x%08X,",_pal4[i]);
	        if ((i & 7) == 7)
	            printf("\n");
	        if (i == 255) {
	            printf("//odd\n");
	        }
	    }
	    printf("};\n");
		*/
	    return _pal4;
	}
}
