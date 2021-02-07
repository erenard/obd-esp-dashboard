
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

#include "emu.h"
#include "math.h"

#define ULONG unsigned int


using namespace std;
// string get_ext(const string& s);
std::string to_string(int i);

static
int gamma_(float v, float g)
{
    if (v < 0)
        return 0;
    v = powf(v,g);
    if (v <= 0.0031308)
        return 0;
    int c = ((1.055 * powf(v, 1.0/2.4) - 0.055)*255);
    return c < 255 ? c : 255;
}


// // make_yuv_palette from RGB palette
// void make_yuv_palette(const char* name, const uint32_t* rgb, int len)
// {
//     uint32_t pal[256*2];
//     uint32_t* even = pal;
//     uint32_t* odd = pal + len;

//     float chroma_scale = BLANKING_LEVEL/2/256;
//     //chroma_scale /= 127;  // looks a little washed out
//     chroma_scale /= 80;
//     for (int i = 0; i < len; i++) {
//         uint8_t r = rgb[i] >> 16;
//         uint8_t g = rgb[i] >> 8;
//         uint8_t b = rgb[i];

//         float y = 0.299 * r + 0.587*g + 0.114 * b;
//         float u = -0.147407 * r - 0.289391 * g + 0.436798 * b;
//         float v =  0.614777 * r - 0.514799 * g - 0.099978 * b;
//         y /= 255.0;
//         y = (y*(WHITE_LEVEL-BLACK_LEVEL) + BLACK_LEVEL)/256;

//         uint32_t e = 0;
//         uint32_t o = 0;
//         for (int i = 0; i < 4; i++) {
//             float p = 2*M_PI*i/4 + M_PI;
//             float s = sin(p)*chroma_scale;
//             float c = cos(p)*chroma_scale;
//             uint8_t e0 = round(y + (s*u) + (c*v));
//             uint8_t o0 = round(y + (s*u) - (c*v));
//             e = (e << 8) | e0;
//             o = (o << 8) | o0;
//         }
//         *even++ = e;
//         *odd++ = o;
//     }

//     printf("uint32_t %s_4_phase_pal[] = {\n",name);
//     for (int i = 0; i < len*2; i++) {  // start with luminance map
//         printf("0x%08X,",pal[i]);
//         if ((i & 7) == 7)
//             printf("\n");
//         if (i == (len-1)) {
//             printf("//odd\n");
//         }
//     }
//     printf("};\n");

//     /*
//      // don't bother with phase tables
//     printf("uint8_t DRAM_ATTR %s[] = {\n",name);
//     for (int i = 0; i < len*(1<<PHASE_BITS)*2; i++) {
//         printf("0x%02X,",yuv[i]);
//         if ((i & 15) == 15)
//             printf("\n");
//         if (i == (len*(1<<PHASE_BITS)-1)) {
//             printf("//odd\n");
//         }
//     }
//     printf("};\n");
//      */
// }

// atari pal palette cc_width==4
static void make_dashboard_4_phase_pal(const uint16_t* lum, const float* angle)
{
    uint32_t _pal4[256*2];
    uint32_t *even = _pal4;
    uint32_t *odd = even + 256;

    float chroma_scale = BLANKING_LEVEL/2/256;
    chroma_scale *= 1.5;
    float s,c,u,v;

    for (int cr = 0; cr < 16; cr++) {
        if (cr == 0) {
            u = v = 0;
        } else {
            u = cos(angle[16-cr]);
            v = sin(angle[16-cr]);
        }
        for (int lm = 0; lm < 16; lm++) {
            uint32_t e = 0;
            uint32_t o = 0;
            for (int i = 0; i < 4; i++) {
                float p = 2*M_PI*i/4;
                s = sin(p)*chroma_scale;
                c = cos(p)*chroma_scale;
                uint8_t e0 = round(lum[lm] + (s*u) + (c*v));
                uint8_t o0 = round(lum[lm] + (s*u) - (c*v));
                e = (e << 8) | e0;
                o = (o << 8) | o0;
            }
            *even++ = e;
            *odd++ = o;
        }
    }

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
}

static int PIN(int x)
{
    if (x < 0) return 0;
    if (x > 255) return 255;
    return x;
}

static uint32_t rgb(int r, int g, int b)
{
    return (PIN(r) << 16) | (PIN(g) << 8) | PIN(b);
}

// lifted from atari800
static void make_dashboard_rgb_palette(uint32_t* palette)
{
    float start_angle =  (303.0f) * M_PI / 180.0f;
    float start_saturation = 0;
    float color_diff = 26.8 * M_PI / 180.0; // color_delay
    float scaled_black_level = (double)16 / 255.0;
    float scaled_white_level = (double)235 / 255.0;
    float contrast = 0;
    float brightness = 0;

    float luma_mult[16]={
        0.6941, 0.7091, 0.7241, 0.7401,
        0.7560, 0.7741, 0.7931, 0.8121,
        0.8260, 0.8470, 0.8700, 0.8930,
        0.9160, 0.9420, 0.9690, 1.0000};

    uint32_t linear[256];
    uint16_t _lum[16];
    float _angle[16];

    for (int cr = 0; cr < 16; cr ++) {
        float angle = start_angle + (cr - 1) * color_diff;
        float saturation = (cr ? (start_saturation + 1) * 0.175f: 0.0);
        float i = cos(angle) * saturation;
        float q = sin(angle) * saturation;

        _angle[cr] = angle + 2*M_PI*33/360;

        for (int lm = 0; lm < 16; lm ++) {
            float y = (luma_mult[lm] - luma_mult[0]) / (luma_mult[15] - luma_mult[0]);
            _lum[lm] = (y*(WHITE_LEVEL-BLACK_LEVEL) + BLACK_LEVEL)/256;

            y *= contrast * 0.5 + 1;
            y += brightness * 0.5;
            y = y * (scaled_white_level - scaled_black_level) + scaled_black_level;

            float gmma = 2.35;
            float r = (y +  0.946882*i +  0.623557*q);
            float g = (y + -0.274788*i + -0.635691*q);
            float b = (y + -1.108545*i +  1.709007*q);
            linear[(cr << 4) | lm] = rgb(r*255,g*255,b*255);
            palette[(cr << 4) | lm] = (gamma_(r,gmma) << 16) | (gamma_(g,gmma) << 8) | (gamma_(b,gmma) << 0);
        }
    }
    make_dashboard_4_phase_pal(_lum,_angle);
}

void make_dashboard_rgb_palette()
{
    uint32_t _dashboard_pal[256];
    make_dashboard_rgb_palette(_dashboard_pal);
    for (int i = 0; i < 256; i++) {
        printf("0x%08X,",_dashboard_pal[i]);
        if ((i & 7) == 7)
            printf("\n");
    }
}

// derived from atari800
static void dashboard_palette(float start_angle = 0)
{
    // float color_inc = 28.6 * M_PI / 180.0;
    #define RAD(_d) ((_d - 180) * M_PI / 180.0)         // (degree - 180) because the color burst is 180d


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
    uint32_t pal[256];
    for (int cr = 0; cr < 8; cr += 1) {
        float angle = color_phase_angles[cr];
        float luminance = color_luminances[cr];
        float amplitude = color_amplitudes[cr];
        for (int lm = 0; lm < 32; lm++) {
            double y = BLACK_LEVEL + (luminance - BLACK_LEVEL) * (lm / 32.0);
            int p[4];
            for (int j = 0; j < 4; j++)
                p[j] = y;
            for (int j = 0; j < 4; j++)
                p[j] += sin(angle + 2.0 * M_PI * j / 4.0) * (amplitude / 2.0) * (lm / 32.0);
            uint32_t pi = 0;
            for (int j = 0; j < 4; j++)
                pi = (pi << 8) | p[j] >> 8;
            pal[i++] = pi;
        }
    }

    uint32_t color;
    // swizzed pattern for esp32 is 0 2 1 3
    printf("const uint32_t _dashboard_4_phase_esp[256] = {\n");
    for (int i = 0; i < 256; i++) {
        color = pal[i];
        color = (color & 0xFF0000FF) | ((color << 8) & 0x00FF0000) | ((color >> 8) & 0x0000FF00);
        printf("0x%08X,",color);
        if ((i & 7) == 7)
            printf("\n");
    }
    printf("};\n");
}


/*
 6502 MPU:
       MOS Technology MCS6502A or equivalent (most NTSC 400/800 machines)
       Atari SALLY (late NTSC 400/800, all PAL 400/800, and all XL/XE)

 CPU CLOCK RATE:
       1.7897725MHz (NTSC machines)
       1.7734470MHz (PAL/SECAM machines)

 FRAME REFRESH RATE:
       59.94Hz (NTSC machines)
       49.86Hz (PAL/SECAM machines)

 MACHINE CYCLES per FRAME:
       29859 (NTSC machines) (1.7897725MHz / 59.94Hz)
       35568 (PAL/SECAM machines) (1.7734470MHz / 49.86Hz)

 SCAN LINES per FRAME
       262 (NTSC machines)
       312 (PAL/SECAM machines)

 MACHINE CYCLES per SCAN LINE
       114        (NTSC machines: 29859 cycles/frame / 262 lines/frame;
              PAL/SECAM machines: 35568 cycles/frame / 312 lines/frame)

 COLOR CLOCKS per MACHINE CYCLE
       2

 COLOR CLOCKS per SCAN LINE
       228  (2 color clocks/machine cycle * 114 machine cycles/scan line)

 MAXIMUM SCAN LINE WIDTH = "WIDE PLAYFIELD"
       176 color clocks

 MAXIMUM RESOLUTION = GRAPHICS PIXEL
       0.5 color clock

 MAXIMUM HORIZONTAL FRAME RESOLUTION
       352 pixels (176 color clocks / 0.5 color clock)

 MAXIMUM VERTICAL FRAME RESOLUTION
       240 pixels (240 scan lines per frame)
 */


//uint32_t _dashboard_pal[256];
const uint32_t dashboard_palette_rgb[256] = {
    0x00000000,0x000F0F0F,0x001B1B1B,0x00272727,0x00333333,0x00414141,0x004F4F4F,0x005E5E5E,
    0x00686868,0x00787878,0x00898989,0x009A9A9A,0x00ABABAB,0x00BFBFBF,0x00D3D3D3,0x00EAEAEA,
    0x00001600,0x000F2100,0x001A2D00,0x00273900,0x00334500,0x00405300,0x004F6100,0x005D7000,
    0x00687A00,0x00778A17,0x00899B29,0x009AAC3B,0x00ABBD4C,0x00BED160,0x00D2E574,0x00E9FC8B,
    0x001C0000,0x00271300,0x00331F00,0x003F2B00,0x004B3700,0x00594500,0x00675300,0x00756100,
    0x00806C12,0x008F7C22,0x00A18D34,0x00B29E45,0x00C3AF56,0x00D6C36A,0x00EAD77E,0x00FFEE96,
    0x002F0000,0x003A0000,0x00460F00,0x00521C00,0x005E2800,0x006C3600,0x007A4416,0x00885224,
    0x00925D2F,0x00A26D3F,0x00B37E50,0x00C48F62,0x00D6A073,0x00E9B487,0x00FDC89B,0x00FFDFB2,
    0x00390000,0x00440000,0x0050000A,0x005C0F17,0x00681B23,0x00752931,0x0084373F,0x0092464E,
    0x009C5058,0x00AC6068,0x00BD7179,0x00CE838A,0x00DF949C,0x00F2A7AF,0x00FFBBC3,0x00FFD2DA,
    0x00370020,0x0043002C,0x004E0037,0x005A0044,0x00661350,0x0074215D,0x0082306C,0x00903E7A,
    0x009B4984,0x00AA5994,0x00BC6AA5,0x00CD7BB6,0x00DE8CC7,0x00F1A0DB,0x00FFB4EF,0x00FFCBFF,
    0x002B0047,0x00360052,0x0042005E,0x004E006A,0x005A1276,0x00672083,0x00762F92,0x00843DA0,
    0x008E48AA,0x009E58BA,0x00AF69CB,0x00C07ADC,0x00D18CED,0x00E59FFF,0x00F9B3FF,0x00FFCAFF,
    0x0016005F,0x0021006A,0x002D0076,0x00390C82,0x0045198D,0x0053279B,0x006135A9,0x006F44B7,
    0x007A4EC2,0x008A5ED1,0x009B6FE2,0x00AC81F3,0x00BD92FF,0x00D0A5FF,0x00E4B9FF,0x00FBD0FF,
    0x00000063,0x0000006F,0x00140C7A,0x00201886,0x002C2592,0x003A329F,0x004841AE,0x00574FBC,
    0x00615AC6,0x00716AD6,0x00827BE7,0x00948CF8,0x00A59DFF,0x00B8B1FF,0x00CCC5FF,0x00E3DCFF,
    0x00000054,0x00000F5F,0x00001B6A,0x00002776,0x00153382,0x00234190,0x0031509E,0x00405EAC,
    0x004A68B6,0x005A78C6,0x006B89D7,0x007D9BE8,0x008EACF9,0x00A1BFFF,0x00B5D3FF,0x00CCEAFF,
    0x00001332,0x00001E3E,0x00002A49,0x00003655,0x00004261,0x0012506F,0x00205E7D,0x002F6D8B,
    0x00397796,0x004987A6,0x005B98B7,0x006CA9C8,0x007DBAD9,0x0091CEEC,0x00A5E2FF,0x00BCF9FF,
    0x00001F00,0x00002A12,0x0000351E,0x0000422A,0x00004E36,0x000B5B44,0x00196A53,0x00287861,
    0x0033826B,0x0043927B,0x0054A38C,0x0065B49E,0x0077C6AF,0x008AD9C2,0x009EEDD6,0x00B5FFED,
    0x00002400,0x00003000,0x00003B00,0x00004700,0x0000530A,0x00106118,0x001E6F27,0x002D7E35,
    0x00378840,0x00479850,0x0059A961,0x006ABA72,0x007BCB84,0x008FDE97,0x00A3F2AB,0x00BAFFC2,
    0x00002300,0x00002F00,0x00003A00,0x00004600,0x00115200,0x001F6000,0x002E6E00,0x003C7C12,
    0x0047871C,0x0057972D,0x0068A83E,0x0079B94F,0x008ACA61,0x009EDD74,0x00B2F189,0x00C9FFA0,
    0x00001B00,0x00002700,0x000F3200,0x001C3E00,0x00284A00,0x00365800,0x00446600,0x00527500,
    0x005D7F00,0x006D8F19,0x007EA02B,0x008FB13D,0x00A0C24E,0x00B4D662,0x00C8EA76,0x00DFFF8D,
    0x00110E00,0x001D1A00,0x00292500,0x00353100,0x00413D00,0x004F4B00,0x005D5A00,0x006B6800,
    0x0076720B,0x0085821B,0x0097932D,0x00A8A43E,0x00B9B650,0x00CCC963,0x00E0DD77,0x00F7F48F,
};

// swizzed ntsc palette in RAM
const uint32_t dashboard_4_phase_ntsc[256] = {
    0x18181818,0x19191919,0x1B1B1B1B,0x1C1C1C1C,0x1E1E1E1E,0x1F1F1F1F,0x21212121,0x22222222,
    0x24242424,0x25252525,0x27272727,0x28282828,0x2A2A2A2A,0x2B2B2B2B,0x2D2D2D2D,0x2E2E2E2E,
    0x30303030,0x32323232,0x33333333,0x35353535,0x36363636,0x38383838,0x39393939,0x3B3B3B3B,
    0x3C3C3C3C,0x3E3E3E3E,0x3F3F3F3F,0x41414141,0x42424242,0x44444444,0x45454545,0x47474747,
    0x18181818,0x19191918,0x1A1A1B19,0x1B1C1D1A,0x1C1D1F1A,0x1D1F211B,0x1F20231C,0x2022251C,
    0x2123271D,0x2225291E,0x23262B1E,0x25282D1F,0x26292F20,0x272B3120,0x282C3321,0x292E3522,
    0x2B2F3723,0x2C313923,0x2D323B24,0x2E343D25,0x2F353F25,0x31374126,0x32384327,0x333A4527,
    0x343B4728,0x353D4929,0x373E4B29,0x38404D2A,0x39414F2B,0x3A43512B,0x3B44532C,0x3D46552D,
    0x18181818,0x19181819,0x1B18191A,0x1D181A1B,0x1F181B1D,0x21181C1E,0x23181D1F,0x25181D20,
    0x27191E22,0x29191F23,0x2B192024,0x2D192126,0x2F192227,0x31192228,0x33192329,0x351A242B,
    0x371A252C,0x391A262D,0x3B1A272F,0x3D1A2730,0x3F1A2831,0x411A2932,0x431A2A34,0x451B2B35,
    0x471B2C36,0x491B2C38,0x4B1B2D39,0x4D1B2E3A,0x4F1B2F3B,0x511B303D,0x531C313E,0x551C323F,
    0x18181818,0x19181918,0x1B181A18,0x1D181C19,0x1E181D19,0x20181E1A,0x2218201A,0x2318211B,
    0x2519221B,0x2719241C,0x2819251C,0x2A19261D,0x2C19281D,0x2D19291E,0x2F192A1E,0x31192C1F,
    0x321A2D1F,0x341A2E1F,0x361A3020,0x371A3120,0x391A3221,0x3B1A3421,0x3D1A3522,0x3E1A3622,
    0x401B3823,0x421B3923,0x431B3A24,0x451B3C24,0x471B3D25,0x481B3E25,0x4A1B4026,0x4C1C4126,
    0x18181818,0x17191819,0x171A181A,0x171C181B,0x171D181C,0x171F181D,0x1720191E,0x1621191F,
    0x16231920,0x16241921,0x16261922,0x16271A23,0x16281A24,0x162A1A25,0x152B1A26,0x152D1A27,
    0x152E1B28,0x152F1B2A,0x15311B2B,0x15321B2C,0x14341B2D,0x14351B2E,0x14361C2F,0x14381C30,
    0x14391C31,0x143B1C32,0x143C1C33,0x133D1D34,0x133F1D35,0x13401D36,0x13421D37,0x13431D38,
    0x18181818,0x17191818,0x161A1918,0x161B1918,0x151D1A18,0x151E1B18,0x141F1B19,0x14211C19,
    0x13221D19,0x13231D19,0x12251E19,0x12261E1A,0x11271F1A,0x1129201A,0x102A201A,0x102B211A,
    0x0F2D221A,0x0F2E221B,0x0E2F231B,0x0E31231B,0x0D32241B,0x0D33251B,0x0C35251C,0x0C36261C,
    0x0B37271C,0x0A39271C,0x0A3A281C,0x093B281C,0x093D291D,0x083E2A1D,0x083F2A1D,0x07412B1D,
    0x18181818,0x18181718,0x18181719,0x1918161A,0x1918161B,0x1918151C,0x1A18151D,0x1A18141D,
    0x1A18141E,0x1B18131F,0x1B181320,0x1B181221,0x1C181222,0x1C181122,0x1C181123,0x1D181024,
    0x1D181025,0x1D181026,0x1E180F27,0x1E180F28,0x1E180E28,0x1F180E29,0x1F180D2A,0x1F180D2B,
    0x20180C2C,0x20180C2D,0x20180B2D,0x21190B2E,0x21190A2F,0x21190A30,0x22190931,0x22190932,
    0x18181818,0x18181718,0x18181719,0x1918161A,0x1918161B,0x1918151C,0x1A18151D,0x1A18141D,
    0x1A18141E,0x1B18131F,0x1B181320,0x1B181221,0x1C181222,0x1C181122,0x1C181123,0x1D181024,
    0x1D181025,0x1D181026,0x1E180F27,0x1E180F28,0x1E180E28,0x1F180E29,0x1F180D2A,0x1F180D2B,
    0x20180C2C,0x20180C2D,0x20180B2D,0x21190B2E,0x21190A2F,0x21190A30,0x22190931,0x22190932,
};

uint32_t *dashboard_4_phase_ntsc_ram = 0;

const uint32_t dashboard_4_phase_pal[] = {
    0x18181818,0x1B1B1B1B,0x1E1E1E1E,0x21212121,0x25252525,0x28282828,0x2B2B2B2B,0x2E2E2E2E,
    0x32323232,0x35353535,0x38383838,0x3B3B3B3B,0x3F3F3F3F,0x42424242,0x45454545,0x49494949,
    0x16271A09,0x192A1D0C,0x1C2D200F,0x1F302312,0x23342716,0x26372A19,0x293A2D1C,0x2C3D301F,
    0x30413423,0x33443726,0x36473A29,0x394A3D2C,0x3D4E4130,0x40514433,0x43544736,0x47584B3A,
    0x0F24210C,0x1227240F,0x152A2712,0x182D2A15,0x1C312E19,0x1F34311C,0x2237341F,0x253A3722,
    0x293E3B26,0x2C413E29,0x2F44412C,0x3247442F,0x364B4833,0x394E4B36,0x3C514E39,0x4055523D,
    0x0B1F2511,0x0E222814,0x11252B17,0x14282E1A,0x182C321E,0x1B2F3521,0x1E323824,0x21353B27,
    0x25393F2B,0x283C422E,0x2B3F4531,0x2E424834,0x32464C38,0x35494F3B,0x384C523E,0x3C505642,
    0x09182718,0x0C1B2A1B,0x0F1E2D1E,0x12213021,0x16253425,0x19283728,0x1C2B3A2B,0x1F2E3D2E,
    0x23324132,0x26354435,0x29384738,0x2C3B4A3B,0x303F4E3F,0x33425142,0x36455445,0x3A495849,
    0x0B11251F,0x0E142822,0x11172B25,0x141A2E28,0x181E322C,0x1B21352F,0x1E243832,0x21273B35,
    0x252B3F39,0x282E423C,0x2B31453F,0x2E344842,0x32384C46,0x353B4F49,0x383E524C,0x3C425650,
    0x0F0C2124,0x120F2427,0x1512272A,0x18152A2D,0x1C192E31,0x1F1C3134,0x221F3437,0x2522373A,
    0x29263B3E,0x2C293E41,0x2F2C4144,0x322F4447,0x3633484B,0x39364B4E,0x3C394E51,0x403D5255,
    0x15091B27,0x180C1E2A,0x1B0F212D,0x1E122430,0x22162834,0x25192B37,0x281C2E3A,0x2B1F313D,
    0x2F233541,0x32263844,0x35293B47,0x382C3E4A,0x3C30424E,0x3F334551,0x42364854,0x463A4C58,
    0x1C0A1426,0x1F0D1729,0x22101A2C,0x25131D2F,0x29172133,0x2C1A2436,0x2F1D2739,0x32202A3C,
    0x36242E40,0x39273143,0x3C2A3446,0x3F2D3749,0x43313B4D,0x46343E50,0x49374153,0x4D3B4557,
    0x220D0E23,0x25101126,0x28131429,0x2B16172C,0x2F1A1B30,0x321D1E33,0x35202136,0x38232439,
    0x3C27283D,0x3F2A2B40,0x422D2E43,0x45303146,0x4934354A,0x4C37384D,0x4F3A3B50,0x533E3F54,
    0x26130A1D,0x29160D20,0x2C191023,0x2F1C1326,0x3320172A,0x36231A2D,0x39261D30,0x3C292033,
    0x402D2437,0x4330273A,0x46332A3D,0x49362D40,0x4D3A3144,0x503D3447,0x5340374A,0x57443B4E,
    0x271A0916,0x2A1D0C19,0x2D200F1C,0x3023121F,0x34271623,0x372A1926,0x3A2D1C29,0x3D301F2C,
    0x41342330,0x44372633,0x473A2936,0x4A3D2C39,0x4E41303D,0x51443340,0x54473643,0x584B3A47,
    0x24200C10,0x27230F13,0x2A261216,0x2D291519,0x312D191D,0x34301C20,0x37331F23,0x3A362226,
    0x3E3A262A,0x413D292D,0x44402C30,0x47432F33,0x4B473337,0x4E4A363A,0x514D393D,0x55513D41,
    0x1F25110B,0x2228140E,0x252B1711,0x282E1A14,0x2C321E18,0x2F35211B,0x3238241E,0x353B2721,
    0x393F2B25,0x3C422E28,0x3F45312B,0x4248342E,0x464C3832,0x494F3B35,0x4C523E38,0x5056423C,
    0x19271709,0x1C2A1A0C,0x1F2D1D0F,0x22302012,0x26342416,0x29372719,0x2C3A2A1C,0x2F3D2D1F,
    0x33413123,0x36443426,0x39473729,0x3C4A3A2C,0x404E3E30,0x43514133,0x46544436,0x4A58483A,
    0x12261E0A,0x1529210D,0x182C2410,0x1B2F2713,0x1F332B17,0x22362E1A,0x2539311D,0x283C3420,
    0x2C403824,0x2F433B27,0x32463E2A,0x3549412D,0x394D4531,0x3C504834,0x3F534B37,0x43574F3B,
    //odd
    0x18181818,0x1B1B1B1B,0x1E1E1E1E,0x21212121,0x25252525,0x28282828,0x2B2B2B2B,0x2E2E2E2E,
    0x32323232,0x35353535,0x38383838,0x3B3B3B3B,0x3F3F3F3F,0x42424242,0x45454545,0x49494949,
    0x1A271609,0x1D2A190C,0x202D1C0F,0x23301F12,0x27342316,0x2A372619,0x2D3A291C,0x303D2C1F,
    0x34413023,0x37443326,0x3A473629,0x3D4A392C,0x414E3D30,0x44514033,0x47544336,0x4B58473A,
    0x21240F0C,0x2427120F,0x272A1512,0x2A2D1815,0x2E311C19,0x31341F1C,0x3437221F,0x373A2522,
    0x3B3E2926,0x3E412C29,0x41442F2C,0x4447322F,0x484B3633,0x4B4E3936,0x4E513C39,0x5255403D,
    0x251F0B11,0x28220E14,0x2B251117,0x2E28141A,0x322C181E,0x352F1B21,0x38321E24,0x3B352127,
    0x3F39252B,0x423C282E,0x453F2B31,0x48422E34,0x4C463238,0x4F49353B,0x524C383E,0x56503C42,
    0x27180918,0x2A1B0C1B,0x2D1E0F1E,0x30211221,0x34251625,0x37281928,0x3A2B1C2B,0x3D2E1F2E,
    0x41322332,0x44352635,0x47382938,0x4A3B2C3B,0x4E3F303F,0x51423342,0x54453645,0x58493A49,
    0x25110B1F,0x28140E22,0x2B171125,0x2E1A1428,0x321E182C,0x35211B2F,0x38241E32,0x3B272135,
    0x3F2B2539,0x422E283C,0x45312B3F,0x48342E42,0x4C383246,0x4F3B3549,0x523E384C,0x56423C50,
    0x210C0F24,0x240F1227,0x2712152A,0x2A15182D,0x2E191C31,0x311C1F34,0x341F2237,0x3722253A,
    0x3B26293E,0x3E292C41,0x412C2F44,0x442F3247,0x4833364B,0x4B36394E,0x4E393C51,0x523D4055,
    0x1B091527,0x1E0C182A,0x210F1B2D,0x24121E30,0x28162234,0x2B192537,0x2E1C283A,0x311F2B3D,
    0x35232F41,0x38263244,0x3B293547,0x3E2C384A,0x42303C4E,0x45333F51,0x48364254,0x4C3A4658,
    0x140A1C26,0x170D1F29,0x1A10222C,0x1D13252F,0x21172933,0x241A2C36,0x271D2F39,0x2A20323C,
    0x2E243640,0x31273943,0x342A3C46,0x372D3F49,0x3B31434D,0x3E344650,0x41374953,0x453B4D57,
    0x0E0D2223,0x11102526,0x14132829,0x17162B2C,0x1B1A2F30,0x1E1D3233,0x21203536,0x24233839,
    0x28273C3D,0x2B2A3F40,0x2E2D4243,0x31304546,0x3534494A,0x38374C4D,0x3B3A4F50,0x3F3E5354,
    0x0A13261D,0x0D162920,0x10192C23,0x131C2F26,0x1720332A,0x1A23362D,0x1D263930,0x20293C33,
    0x242D4037,0x2730433A,0x2A33463D,0x2D364940,0x313A4D44,0x343D5047,0x3740534A,0x3B44574E,
    0x091A2716,0x0C1D2A19,0x0F202D1C,0x1223301F,0x16273423,0x192A3726,0x1C2D3A29,0x1F303D2C,
    0x23344130,0x26374433,0x293A4736,0x2C3D4A39,0x30414E3D,0x33445140,0x36475443,0x3A4B5847,
    0x0C202410,0x0F232713,0x12262A16,0x15292D19,0x192D311D,0x1C303420,0x1F333723,0x22363A26,
    0x263A3E2A,0x293D412D,0x2C404430,0x2F434733,0x33474B37,0x364A4E3A,0x394D513D,0x3D515541,
    0x11251F0B,0x1428220E,0x172B2511,0x1A2E2814,0x1E322C18,0x21352F1B,0x2438321E,0x273B3521,
    0x2B3F3925,0x2E423C28,0x31453F2B,0x3448422E,0x384C4632,0x3B4F4935,0x3E524C38,0x4256503C,
    0x17271909,0x1A2A1C0C,0x1D2D1F0F,0x20302212,0x24342616,0x27372919,0x2A3A2C1C,0x2D3D2F1F,
    0x31413323,0x34443626,0x37473929,0x3A4A3C2C,0x3E4E4030,0x41514333,0x44544636,0x48584A3A,
    0x1E26120A,0x2129150D,0x242C1810,0x272F1B13,0x2B331F17,0x2E36221A,0x3139251D,0x343C2820,
    0x38402C24,0x3B432F27,0x3E46322A,0x4149352D,0x454D3931,0x48503C34,0x4B533F37,0x4F57433B,
};
uint32_t *dashboard_4_phase_pal_ram = 0;

ULONG *Screen_dashboard;

#define Screen_WIDTH  384
#define Screen_HEIGHT 240

//========================================================================================
//========================================================================================

class Dashboard : public Emu {
    uint8_t** _lines;
public:
    Dashboard(int ntsc) : Emu("dashboard", Screen_WIDTH, Screen_HEIGHT, ntsc, (16 | (1 << 8)), 4, EMU_DASHBOARD)
    {
        _lines = 0;
        init_screen();
        gen_palettes();
    }

    virtual void gen_palettes()
    {
        // make_dashboard_rgb_palette();
        dashboard_palette();
    }

    // 32 bit aligned access
    inline uint8_t getp(const uint8_t* src)
    {
        size_t s = (size_t)src;
        return *((uint32_t*)(s & ~3)) >> ((s & 3) << 3);
    }

    inline void setp(uint8_t* dst, uint8_t p)
    {
        size_t s = (size_t)dst;
        size_t shift = (s & 3) << 3;
        uint32_t* d = (uint32_t*)(s & ~3);
        d[0] = (d[0] & ~(0xFF << shift)) | (p << shift);
    }

    // Used to find margins of a given screen
    void test_pattern()
    {
        // Plain white
        for (int y = 0; y < Screen_HEIGHT; y++) {
            uint8_t* line = (uint8_t*) _lines[y];
            for (int x = 0; x < Screen_WIDTH; x++) {
                setp(line + x, 15);
            }
        }

        int margin_top = 13;
        int margin_left = 9;
        int margin_bottom = 9;
        int margin_right = 19;
        // 356x218 after margins
        int first_cell_number = 0;
        int cell_number = 0;
        int cell_width = 1 + (Screen_WIDTH - margin_left - margin_right) / 16;
        int cell_height = (Screen_HEIGHT - margin_top - margin_bottom) / 16;
        for (int y = margin_top; y < Screen_HEIGHT - margin_bottom; y++) {
            uint8_t* line = (uint8_t*) _lines[y];
            if ((y - margin_top) % cell_height) {
                cell_number = first_cell_number;
                for (int x = margin_left; x < Screen_WIDTH - margin_right; x++) {
                    if ((x - margin_left) % cell_width) {
                        setp(line + x, cell_number % 256);
                    } else {
                        setp(line + x, 0);
                        setp(line + x - 1, 0);
                        cell_number++;
                    }
                }
            } else {
                for (int x = margin_left; x < Screen_WIDTH - margin_right; x++) {
                    setp(line + x, 0);
                }
                first_cell_number = cell_number;
            }
        }
    }

    // allocate most of the big stuff in 32 bit  mem
    void init_screen()
    {
        Screen_dashboard = (ULONG*) MALLOC32(Screen_WIDTH * Screen_HEIGHT, "Screen_dashboard");    // 32 bit access plz
        _lines = (uint8_t**) MALLOC32(height * sizeof(uint8_t*), "_lines");
        const uint8_t* s = (uint8_t*) Screen_dashboard;
        for (int y = 0; y < height; y++) {
            _lines[y] = (uint8_t*) s;
            s += width;
        }
        clear_screen();
        test_pattern();
    }

    void clear_screen()
    {
        int i = Screen_WIDTH*Screen_HEIGHT/4;
        while (i--)
            Screen_dashboard[i] = 0;
    }

    virtual int update()
    {
        // return libatari800_next_frame(NULL);
        // UPDATE Dashboard here.
        return 0;
    }

    virtual uint8_t** video_buffer()
    {
        return _lines;
    }

    virtual const uint32_t* ntsc_palette()
    {
      if (!dashboard_4_phase_ntsc_ram) {
            dashboard_4_phase_ntsc_ram = new uint32_t[256];
            memcpy(dashboard_4_phase_ntsc_ram,dashboard_4_phase_ntsc,256*4);  // copy into ram as we are tight on static mem
      }
      return dashboard_4_phase_ntsc_ram;
    };

    virtual const uint32_t* pal_palette()
    {
        if (!dashboard_4_phase_pal_ram)
        {
            dashboard_4_phase_pal_ram = new uint32_t[512];
            memcpy(dashboard_4_phase_pal_ram,dashboard_4_phase_pal,512*4);  // copy into ram as we are tight on static mem
        }
        return dashboard_4_phase_pal_ram;
     }
    virtual const uint32_t* rgb_palette()   { return dashboard_palette_rgb; };
};

Emu* NewDashboard(int ntsc)
{
    return new Dashboard(ntsc);
}
