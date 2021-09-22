#include "id666.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

static const char *text = "text";
static const char *binary = "binary";

static void id6_dump(id666 *id6) {
    fprintf(stdout,"type: %s\n", (id6->binary ? binary : text));
    fprintf(stdout,"song: %s\n",id6->song);
    fprintf(stdout,"game: %s\n",id6->game);
    fprintf(stdout,"dumper: %s\n",id6->dumper);
    fprintf(stdout,"comment: %s\n",id6->comment);
    fprintf(stdout,"artist: %s\n",id6->artist);
    fprintf(stdout,"publisher: %s\n",id6->publisher);
    fprintf(stdout,"ost: %s\n",id6->ost);
    fprintf(stdout,"year: %u\n",id6->year);
    fprintf(stdout,"total_len: %d\n",id6->total_len);
    fprintf(stdout,"play_len: %d\n",id6->play_len);
    fprintf(stdout,"len: %d\n",id6->len);
    fprintf(stdout,"intro: %d\n",id6->intro);
    fprintf(stdout,"loop: %d\n",id6->loop);
    fprintf(stdout,"end: %d\n",id6->end);
    fprintf(stdout,"fade: %d\n",id6->fade);
    fprintf(stdout,"loops: %hhu\n",id6->loops);
    fprintf(stdout,"mute: %hhu\n",id6->mute);
    fprintf(stdout,"ost_disc: %hhu\n",id6->ost_disc);
    fprintf(stdout,"ost_track: %hhu\n",id6->ost_track);
    fprintf(stdout,"emulator: %hhu\n",id6->emulator);
    fprintf(stdout,"amp: %u (%f)\n",id6->amp, ((float)id6->amp) / 65536.0);
    fprintf(stdout,"rip_year: %d\n",id6->rip_year);
    fprintf(stdout,"rip_month: %d\n",id6->rip_month);
    fprintf(stdout,"rip_day: %d\n",id6->rip_day);
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
        id6_dump(id6);
    }
    fprintf(stdout,"\n");

    free(id6);
    free(data);
    return 0;
}
