#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include <unistd.h>
#include <ncurses.h>

#include "sync.h"

static double _row = 0.0;
static double _row_add = 1.0;

static double bass_get_row(void *)
{
    double ret_val = _row;

    printf("bass_get_row: %d\n", ret_val);

    _row += _row_add;

    return ret_val;
}

#ifndef SYNC_PLAYER

static void bass_pause(void *v, int i)
{
    if (i)
    {
        _row_add = 0.0;
    }
    else
    {
        _row_add = 1.0;
    }
    printf("bass_pause: %d\n", i);
}

static void bass_set_row(void *v, int i)
{
    _row = i;
    printf("bass_set_row %f\n", _row);
}

static int bass_is_playing(void *v)
{
    printf("bass_is_playing %f\n", _row_add);
    return _row_add;
}

static struct sync_cb bass_cb = {
    bass_pause,
    bass_set_row,
    bass_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

int main(int argc, char *argv[])
{
    sync_device *rocket = sync_create_device("s");
    if (!rocket)
        printf("out of memory?\n");

#ifndef SYNC_PLAYER
    if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
        printf("failed to connect to host\n");
#endif

    /* get tracks */
    const sync_track *a_a = sync_get_track(rocket, "a:a");
    const sync_track *a_b = sync_get_track(rocket, "a:b");
    const sync_track *a_c = sync_get_track(rocket, "a:c");
    const sync_track *b_a = sync_get_track(rocket, "b:a");
    const sync_track *b_b = sync_get_track(rocket, "b:b");

    while (ERR == getch())
    {
        double row = bass_get_row(NULL);

#ifndef SYNC_PLAYER
        if (sync_update(rocket, (int)floor(row), &bass_cb, NULL))
            sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

        printf("a.a:%f a.b:%f a.c:%f\n",
                sync_get_val(a_a, row),
                sync_get_val(a_b, row),
                sync_get_val(a_c, row));

        printf("b:a:%f b:b:%f\n",
                sync_get_val(b_a, row),
                sync_get_val(b_b, row));
    }

#ifndef SYNC_PLAYER
    sync_save_tracks(rocket);
#endif
    sync_destroy_device(rocket);

    return 0;
}
