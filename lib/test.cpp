#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "device.h"
#include "sync.h"

static double bass_get_row(void *v)
{
    printf("bass_get_row\n");
    return 0.0;
}

#ifndef SYNC_PLAYER

static void bass_pause(void *v, int i)
{
    printf("bass_pause\n");
}

static void bass_set_row(void *v, int i)
{
    printf("bass_set_row\n");
}

static int bass_is_playing(void *v)
{
    printf("bass_is_playing\n");
    return 1;
}

static struct sync_cb bass_cb = {
    bass_pause,
    bass_set_row,
    bass_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

void setup_sdl(void)
{
    printf("setup_sdl\n");
}

void draw_cube()
{
    printf("draw_cube\n");
}

int main(int argc, char *argv[])
{
    sync_device *rocket = sync_create_device("sync");
    if (!rocket)
        printf("out of memory?\n");

#ifndef SYNC_PLAYER
    if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
        printf("failed to connect to host\n");
#endif

    /* get tracks */
    const sync_track *clear_r = sync_get_track(rocket, "clear.r");
    const sync_track *clear_g = sync_get_track(rocket, "clear.g");
    const sync_track *clear_b = sync_get_track(rocket, "clear.b");
    const sync_track *cam_rot = sync_get_track(rocket, "camera:rot.y");
    const sync_track *cam_dist = sync_get_track(rocket, "camera:dist");

    while (!kbhit())
    {
        double row = bass_get_row(NULL);

#ifndef SYNC_PLAYER
        if (sync_update(rocket, (int)floor(row), &bass_cb, NULL))
            sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

        printf("r:%f g:%f b:%f\n",
                sync_get_val(clear_r, row),
                sync_get_val(clear_g, row),
                sync_get_val(clear_b, row));

        float rot = sync_get_val(cam_rot, row);
        float dist = sync_get_val(cam_dist, row);

        printf("rot:%f dist:%f\n", rot, dist);
    }

#ifndef SYNC_PLAYER
    sync_save_tracks(rocket);
#endif
    sync_destroy_device(rocket);

    return 0;
}
