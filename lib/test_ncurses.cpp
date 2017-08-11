#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include <unistd.h>
#include <ncurses.h>

#include "sync.h"

#if 0
#define TIME_INC (1.0/60.0)
#else
#define TIME_INC (1.0)
#endif

static const float bpm = 60.0f; /* beats per minute */
static const int rpb = 1; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * rpb;

static double time = 0.0;
static double time_inc = TIME_INC;

static double bass_get_row(void *)
{
    double ret_val = time * row_rate;
    printw("bass_get_row: %f\n", ret_val);
	return ret_val;
}

#ifndef SYNC_PLAYER

static void bass_pause(void *v, int i)
{
    if (i)
    {
        time_inc = 0.0;
    }
    else
    {
        time_inc = TIME_INC;
    }
    printw("bass_pause: %d\n", i);
}

static void bass_set_row(void *v, int row)
{
    time = row / row_rate;
    printw("bass_set_row time: %f, row: %d\n", time, row);
}

static int bass_is_playing(void *v)
{
    int ret_val = TIME_INC == time_inc;
    printw("bass_is_playing %d\n", ret_val);
    return ret_val;
}

static struct sync_cb bass_cb = {
    bass_pause,
    bass_set_row,
    bass_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

int main(int argc, char *argv[])
{
    initscr();
    cbreak();
    noecho();
    scrollok(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    sync_device *rocket = sync_create_device("s");
    if (!rocket)
        printw("out of memory?\n");

#ifndef SYNC_PLAYER
    if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
        printw("failed to connect to host\n");
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

        if (row >= 0x80) break;

#ifndef SYNC_PLAYER
        if (sync_update(rocket, /*(int)floor(row)*/row, &bass_cb, NULL))
            sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

        printw("a.a:%f a.b:%f a.c:%f\n",
                sync_get_val(a_a, row),
                sync_get_val(a_b, row),
                sync_get_val(a_c, row));

        printw("b:a:%f b:b:%f\n",
                sync_get_val(b_a, row),
                sync_get_val(b_b, row));

        time += time_inc;
    }

#ifndef SYNC_PLAYER
    sync_save_tracks(rocket);
#endif
    sync_destroy_device(rocket);

    return 0;
}
