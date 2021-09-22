#include "id666.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <gme/gme.h>

#define STRING_TAG_CHECK(id6_tagid,gme_tagid) \
    if(strcmp(id6->id6_tagid , info->gme_tagid ) != 0) { \
        printf("tag error (%s): we got '%s', gme got '%s'\n", \
          #id6_tagid, \
          id6->id6_tagid, \
          info->gme_tagid); \
        return 1; \
    }

/* gme uses milliseconds, id6 uses ticks */
#define LENGTH_TAG_CHECK(id6_tagid,gme_tagid) \
    if((int)(id6->id6_tagid / 64) != (int)(info->gme_tagid)) { \
        printf("tag error (%s): we got '%d', gme got '%d'\n", \
          #id6_tagid, \
          (int)(id6->id6_tagid / 64), \
          (int)info->gme_tagid); \
        return 1; \
    }

int compare_gme(id666 *id6, const char *filename) {
    gme_t *Emu = NULL;
    gme_err_t err = NULL;
    gme_info_t *info = NULL;

    err = gme_open_file(filename, &Emu, gme_info_only);
    if(Emu == NULL) return 1;
    if(err != NULL) {
        printf("gme error: %s\n",err);
        return 1;
    }
    gme_track_info(Emu,&info,0);
    STRING_TAG_CHECK(game,game)
    STRING_TAG_CHECK(song,song)
    STRING_TAG_CHECK(artist,author)
    STRING_TAG_CHECK(dumper,dumper)
    STRING_TAG_CHECK(comment,comment)

    LENGTH_TAG_CHECK(fade,fade_length)
    LENGTH_TAG_CHECK(len,play_length)

    printf("gme/id666 tags match\n");

    gme_free_info(info);
    gme_delete(Emu);
    return 0;
}

int main(int argc, char *argv[]) {
    FILE *f;
    size_t len;
    jpr_uint8 *data;
    id666 *id6;
    if(argc < 2) {
        printf("Usage: id666 /path/to/spc\n");
        return 1;
    }
    f = fopen(argv[1],"rb");
    if(f == NULL) return 1;
    fseek(f,0,SEEK_END);
    len = ftell(f);
    fseek(f,0,SEEK_SET);

    data = (jpr_uint8 *)malloc(len);
    if(data == NULL) return 1;
    fread(data,1,len,f);
    fclose(f);

    printf("## %s ##\n",basename(argv[1]));
    id6 = (id666 *)malloc(sizeof(id666));
    if(id6 == NULL) return 1;
    if(id666_parse(id6,data,len) == 0) {
        compare_gme(id6,argv[1]);
    }
    fprintf(stdout,"\n");

    free(id6);
    free(data);
    return 0;
}
