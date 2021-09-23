#include "attr.h"
#include "id666.h"
#include <limits.h>

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

#ifdef ID666_NO_STDLIB
attr_pure
static size_t str_len(const char *s);

attr_pure
static int mem_cmp(const void *p1, const void *p2, size_t n);

static void *mem_cpy(void *dest, const void *src, size_t n);

static char *str_cpy(char *d, const char *p);
#else
#include <string.h>
#define str_len strlen
#define mem_cmp memcmp
#define mem_cpy memcpy
#define str_cpy strcpy
#endif

static size_t scan_uint64(const char *str, jpr_uint64 *num);
static size_t scan_uint(const char *s, unsigned int *num);
static jpr_uint32 unpack_uint32le(const jpr_uint8 *b);
static jpr_int32 unpack_int32le(const jpr_uint8 *b);
static jpr_uint16 unpack_uint16le(const jpr_uint8 *b);

static const char *header_str = "SNES-SPC700 Sound File Data v0.30";

#ifdef ID666_NO_STDLIB
static size_t str_len(const char *s) {
    const char *a = s;
    while(*s) s++;
    return s - a;
}

static int mem_cmp(const void *p1, const void *p2, size_t n) {
    const jpr_uint8 *l;
    const jpr_uint8 *r;

    size_t m = n / sizeof(size_t);

    const size_t *ll = (const size_t  *)p1;
    const size_t *lr = (const size_t  *)p2;

    while(m) {
        if(*ll != *lr) break;
        m--;
        ll++;
        lr++;
        n-=sizeof(size_t);
    }

    l = (const jpr_uint8 *)ll;
    r = (const jpr_uint8 *)lr;

    while(n && *l == *r) {
        n--;
        l++;
        r++;
    }
    return n ? *l - *r : 0;
}

static void *mem_cpy(void *dest, const void *src, size_t n) {
#ifdef _MSC_VER
    void *d = dest;
    __movsb(d,src,n);
#elif defined(__i386__) || defined(__x86_64__)
    void *d = dest;
    __asm__ __volatile("rep movsb" : "=D"(d), "=S"(src), "=c"(n) : "0"(d), "1"(src), "2"(n) : "memory");
#else
    jpr_uint8 *d;
    const jpr_uint8 *s;
    size_t m = n / sizeof(size_t);

    size_t *ld = (size_t  *)dest;
    const size_t *ls = (const size_t  *)src;

    while(m) {
        *ld = *ls;
        ld++;
        ls++;
        m--;
        n-=sizeof(size_t);
    }

    d = (jpr_uint8 *)ld;
    s = (const jpr_uint8 *)ls;

    while(n--) {
        *d = *s;
        d++;
        s++;
    }
#endif
    return dest;
}

static char *str_cpy(char *d, const char *s) {
    char *dest = d;
    while((*d = *s) != 0) {
        s++;
        d++;
    }
    *d = 0;
    return dest;
}
#endif

static size_t scan_uint64(const char *str, jpr_uint64 *num) {
    const char *s = str;
    *num = 0;
    while(*s) {
        if(*s < 48 || *s > 57) break;
        *num *= 10;
        *num += (*s - 48);
        s++;
    }

    return s - str;
}


static size_t scan_uint(const char *str, unsigned int *num) {
    size_t r = 0;
    jpr_uint64 t;
    r = scan_uint64(str, &t);
	if(t > UINT_MAX) t = UINT_MAX;
    *num = (unsigned int)t;
    return r;
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static jpr_uint32 unpack_uint32le(const jpr_uint8 *b) {
    return ((jpr_uint32 *)b)[0];
}

static jpr_int32 unpack_int32le(const jpr_uint8 *b) {
    return ((jpr_int32 *)b)[0];
}

static jpr_uint16 unpack_uint16le(const jpr_uint8 *b) {
    return ((jpr_uint16 *)b)[0];
}

#else

static jpr_uint32 unpack_uint32le(const jpr_uint8 *b) {
    return (((jpr_uint32)b[3])<<24) |
           (((jpr_uint32)b[2])<<16) |
           (((jpr_uint32)b[1])<<8 ) |
           (((jpr_uint32)b[0])    );
}

static jpr_int32 unpack_int32le(const jpr_uint8 *b) {
	jpr_int32 r;
	r  = (jpr_int32) ((jpr_int8)b[3])   << 24;
	r |= (jpr_int32) ((jpr_uint32)b[2]) << 16;
	r |= (jpr_int32) ((jpr_uint32)b[1]) <<  8;
	r |= (jpr_int32) ((jpr_uint32)b[0])      ;
	return r;

}

static jpr_uint16 unpack_uint16le(const jpr_uint8 *b) {
    return (((jpr_uint16)b[1])<<8) |
           (((jpr_uint16)b[0])   );
}


#endif


static void trim_whitespace(jpr_uint8 *str) {
    size_t len = str_len((const char *)str);
    while(len--) {
        if(str[len] == 0x00) {
            return;
        }
        else if(str[len] < 0x21) {
            str[len] = 0x00;
        }
        else {
            return;
        }
    }
}

/* checks if data is '/'-'9' */
static int is_text(jpr_uint8 *data, size_t len) {
    size_t i = 0;
    for(i=0;i<len;i++) {
        if(data[i] > 0 && (data[i] < 45 || data[i] > 57)) {
            return 0;
        }
    }
    return 1;
}

static int is_zero(jpr_uint8 *data, size_t len) {
    size_t i = 0;
    for(i=0;i<len;i++) {
        if(data[i] > 0) {
            return 0;
        }
    }
    return 1;
}

static void id666_load_date(id666 *id6, jpr_uint32 date) {
    if(date > 0) {
        id6->rip_day = date % 100;
        date /= 100;
        id6->rip_month = date % 100;
        date /= 100;
        id6->rip_year  = date;
    }
}

static void id666_load_xid6(id666 *id6, jpr_uint8 *data, size_t len) {
    unsigned int i_tmp = 0;
    size_t offset = 0;
    char t_tmp[261];
    t_tmp[260] = 0;

    while(len > 0) {
        t_tmp[4] = 0;
        mem_cpy(t_tmp,&data[offset],4);
        i_tmp = (unsigned int)unpack_uint16le((jpr_uint8 *)&t_tmp[2]);
        offset += 4;
        len -= 4;
        if(t_tmp[1] == 0) { /* data immediately follows */
            switch(t_tmp[0]) {
                case 0x06: id6->emulator = (jpr_uint8)i_tmp; break;
                case 0x11: id6->ost_disc = (jpr_uint8)i_tmp; break;
                case 0x12: id6->ost_track = t_tmp[3]; break;
                case 0x14: id6->year = (int)i_tmp; break;
                case 0x34: id6->mute = (jpr_uint8)i_tmp; break;
                case 0x35: id6->loops = (jpr_uint8)i_tmp; break;
            }
        } else { /* need to read in data */
            if(len && len >= i_tmp) {
                mem_cpy(&t_tmp[4],&data[offset],i_tmp);
                t_tmp[4+i_tmp] = 0;
                switch(t_tmp[0]) {
                    case 0x01: str_cpy(id6->song,&t_tmp[4]);   trim_whitespace((jpr_uint8 *)id6->song); break;
                    case 0x02: str_cpy(id6->game,&t_tmp[4]);   trim_whitespace((jpr_uint8 *)id6->game); break;
                    case 0x03: str_cpy(id6->artist,&t_tmp[4]); trim_whitespace((jpr_uint8 *)id6->artist); break;
                    case 0x04: str_cpy(id6->dumper,&t_tmp[4]); trim_whitespace((jpr_uint8 *)id6->dumper); break;
                    case 0x05: id666_load_date(id6,unpack_uint32le((jpr_uint8 *)&t_tmp[4])); break;
                    case 0x07: str_cpy(id6->comment,&t_tmp[4]);   trim_whitespace((jpr_uint8 *)id6->comment); break;
                    case 0x10: str_cpy(id6->ost,&t_tmp[4]);       trim_whitespace((jpr_uint8 *)id6->ost); break;
                    case 0x13: str_cpy(id6->publisher,&t_tmp[4]); trim_whitespace((jpr_uint8 *)id6->publisher); break;
                    case 0x30: id6->intro = (int)unpack_uint32le((jpr_uint8 *)&t_tmp[4]); break;
                    case 0x31: id6->loop = (int)unpack_uint32le((jpr_uint8 *)&t_tmp[4]);  break;
                    case 0x32: id6->end  = unpack_int32le((jpr_uint8 *)&t_tmp[4]); break;
                    case 0x33: id6->fade = (int)unpack_uint32le((jpr_uint8 *)&t_tmp[4]); break;
                    case 0x36: id6->amp = unpack_uint32le((jpr_uint8 *)&t_tmp[4]); break;
                }
                i_tmp = (i_tmp + 3) & ~0x03;
                offset += i_tmp;
                len -= i_tmp;
            } else {
                len = 0;
            }
        }
    }

}

int id666_parse(id666 *id6, jpr_uint8 *data, size_t len) {
    char t_tmp[33];
    int t_date, t_len, t_fade;
    jpr_uint8 binary;
    unsigned int date;
    unsigned int offset;
    size_t rem;
    jpr_int32 total_len;

    binary = 0;
    offset = 0;
    rem = 0;

    if(len < 0x100) return 1;
    if(mem_cmp(data,header_str,str_len(header_str)) != 0) return 1;
    if(data[0x23] == 27) return 1;

    id6->song[0] = '\0';
    id6->game[0] = '\0';
    id6->dumper[0] = '\0';
    id6->comment[0] = '\0';
    id6->artist[0] = '\0';
    id6->ost[0] = '\0';
    id6->rip_year = -1;
    id6->rip_month = -1;
    id6->rip_day = -1;
    id6->year = -1;
    id6->len = -1;
    id6->intro = -1;
    id6->loop = -1;
    id6->end = 0;
    id6->fade = -1;
    id6->mute = 0;
    id6->loops = 0;
    id6->ost_disc = 0;
    id6->ost_track = 0;
    id6->emulator = 0;
    id6->amp = 65536;

    t_tmp[32] = 0;
    mem_cpy(t_tmp,&data[0x2E],32);
    str_cpy(id6->song,t_tmp);
    trim_whitespace((jpr_uint8 *)id6->song);

    mem_cpy(t_tmp,&data[0x4E],32);
    str_cpy(id6->game,t_tmp);
    trim_whitespace((jpr_uint8 *)id6->game);

    mem_cpy(t_tmp,&data[0x7E],32);
    str_cpy(id6->comment,t_tmp);
    trim_whitespace((jpr_uint8 *)id6->comment);

    t_tmp[16] = 0;
    mem_cpy(t_tmp,&data[0x6E],16);
    str_cpy(id6->dumper,t_tmp);
    trim_whitespace((jpr_uint8 *)id6->dumper);

    /* now to figure out if this is a text or binary tag */
    t_date = is_text(&data[0x9E],11);
    t_len  = is_text(&data[0xA9],3);
    t_fade = is_text(&data[0xAC],5);

    if(t_date == 0) {
        binary = 1;
    } else {
        /* t_date can be 1 because the entire field is zeros */
        if(t_len == 0 && t_fade == 0) {
            binary = 1;
        }
        if(t_len == 1 && t_fade == 1) {
            if(data[0xB0] == 0 || (data[0xB0] > 47 && data[0xB0] < 58)) {
                binary = 0;
            }
        }
        else {
            if(is_zero(&data[0xA2],7)) {
                binary = 1;
            }
        }
    }

    if(binary) {
        id6->emulator = data[0xD1];
        t_tmp[5] = 0;
        mem_cpy(t_tmp,&data[0x9E],4);
        id666_load_date(id6,unpack_uint32le((jpr_uint8 *)t_tmp));
        t_tmp[4] = 0;
        mem_cpy(t_tmp,&data[0xA9],3);
        id6->len = (int)(unpack_uint32le((jpr_uint8 *)t_tmp) * 64000);
        t_tmp[5] = 0;
        mem_cpy(t_tmp,&data[0xAC],4);
        id6->fade = (int)(unpack_uint32le((jpr_uint8 *)t_tmp) * 64);

        mem_cpy(t_tmp,&data[0xB0],32);

    } else {
        id6->emulator = data[0xD2];
        t_tmp[11] = 0;
        mem_cpy(t_tmp,&data[0x9E],11);

        offset = 0;
        rem = scan_uint(&t_tmp[offset],&date);
        if(rem) {
            offset += rem + 1;
            id6->rip_month = date;
        }

        if(offset) {
            rem = scan_uint(&t_tmp[offset],&date);
            if(rem) {
                offset += rem + 1;
                id6->rip_day = date;
            } else {
                offset = 0;
            }
        }

        if(offset) {
            rem = scan_uint(&t_tmp[offset],&date);
            if(rem) {
                id6->rip_year = date;
            }
        }

        t_tmp[3] = 0;
        mem_cpy(t_tmp,&data[0xA9],3);
        if(scan_uint(t_tmp,&date)) {
            id6->len = (int)date * 64000;
        }

        t_tmp[5] = 0;
        mem_cpy(t_tmp,&data[0xAC],5);
        if(scan_uint(t_tmp,&date)) {
            id6->fade = (int)date * 64;
        }

        mem_cpy(t_tmp,&data[0xB1],32);

    }
    t_tmp[32] = 0;
    str_cpy(id6->artist,t_tmp);
    trim_whitespace((jpr_uint8 *)id6->artist);

    /* check for extended info */
    if(len > 0x10200) {
        offset = 0x10200;
        t_tmp[8] = 0;
        mem_cpy(t_tmp,&data[offset],8);
        if(mem_cmp(t_tmp,"xid6",4) == 0) {
            rem = unpack_uint32le((jpr_uint8 *)&t_tmp[4]);
            offset += 8;
            id666_load_xid6(id6,  &data[offset],rem);
        }
    }

    if(id6->intro == -1 &&
       id6->loop  == -1 &&
       id6->end   == 0) {
        total_len = id6->len;
    } else {
        total_len = 0;
        if(id6->intro != -1) {
            total_len += id6->intro;
        }
        if(id6->loop != -1) {
            total_len += (id6->loop * id6->loops);
        }
        total_len += id6->end;
    }
    id6->play_len = total_len;
    id6->total_len = total_len + id6->fade;
    id6->binary = binary;

    return 0;
}

