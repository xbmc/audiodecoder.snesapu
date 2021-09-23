#ifndef ID666_H
#define ID666_H

/*
The MIT License

Copyright (c) 2020 John Regan <john@jrjrtech.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stddef.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef   signed char    jpr_int8;
typedef unsigned char    jpr_uint8;
typedef   signed short   jpr_int16;
typedef unsigned short   jpr_uint16;
typedef   signed int     jpr_int32;
typedef size_t     jpr_uint32;
typedef   signed __int64 jpr_int64;
typedef unsigned __int64 jpr_uint64;
#else
#include <stdint.h>
typedef int8_t           jpr_int8;
typedef uint8_t          jpr_uint8;
typedef int16_t          jpr_int16;
typedef uint16_t         jpr_uint16;
typedef int32_t          jpr_int32;
typedef uint32_t         jpr_uint32;
typedef int64_t          jpr_int64;
typedef uint64_t         jpr_uint64;
#endif

typedef struct id666_s id666;

struct id666_s {
    /* song title - if song[0]==0, unknown */
    char song[256];

    /* game title - if game[0]==0, unknown */
    char game[256];

    /* dumper - if dumper[0]==0, unknown */
    char dumper[256];

    /* comment - if comment[0]==0, unknown */
    char comment[256];

    /* artist - if artist[0]==0, unknown */
    char artist[256];

    /* publisher - if publisher[0]==0, unknown */
    char publisher[256];

    /* official ost name - if ost[0]==0, unknown */
    char ost[256];

    /* rip year/month/day, -1 == not found */
    int rip_year;
    int rip_month;
    int rip_day;

    /* publish year, -1 == not found */
    int year;

    /* all times given in "ticks", 1 tick == 1/64000 seconds */
    /* play length (in ticks) (computed) - can be ( intro + (loop * loops) + end) or just len */
    jpr_int32 play_len;

    /* total length (in ticks) (computed) = play_len + fade */
    jpr_int32 total_len;

    /* song length (in ticks) (from the file) */
    /* can be zero, check intro length if len == 0 */
    jpr_int32 len;

    /* fade length (in ticks) (from the file) */
    jpr_int32 fade;

    /* intro length (in ticks) (from the file) */
    /* -1 indicates not in file */
    jpr_int32 intro;

    /* loop length (in ticks) (from the file) */
    /* -1 indicates not in file */
    jpr_int32 loop;

    /* end length (in ticks) (from the file) */
    /* 0 indicates not in file (can be negative) */
    jpr_int32 end;

    /* voice(s) to mute, check bits */
    jpr_uint8 mute;

    /* number of times to loop */
    jpr_uint8 loops;

    /* if there's an official OST - disc number. */
    /* 0 = unknown */
    jpr_uint8 ost_disc;

    /* if there's an official OST - track number. */
    /* 0 = unknown */
    jpr_uint8 ost_track;

    /* emulator used to dump spc */
    jpr_uint8 emulator;

    /* mixing amplitude/gain - given as a -15.16 packed integer */
    /* default is 65536 (1 << 16) */
    jpr_uint32 amp;

    /* were the id666 tags in binary format (1) or text format (0) */
    jpr_uint8 binary;
};

#ifdef __cplusplus
extern "C" {
#endif

int id666_parse(id666 *id6, jpr_uint8 *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
