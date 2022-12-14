// SPDX-FileCopyrightText: 2014-2022 Oswald Buddenhagen <ossi@users.sf.net>
// SPDX-License-Identifier: GPL-2.0-or-later
//
// isync test suite
//

#include "common.h"

typedef struct {
	int id;
	int first, other, morph_at, morph_to;
	int64_t start;
	wakeup_t timer;
	wakeup_t morph_timer;
} tst_t;

static void
timer_start( tst_t *timer, int to )
{
	printf( "starting timer %d, should expire after %d\n", timer->id, to );
	timer->start  = get_now();
	conf_wakeup( &timer->timer, to );
}

static void
timed_out( void *aux )
{
	tst_t *timer = (tst_t *)aux;

	printf( "timer %d expired after %d, repeat %d\n",
	        timer->id, (int)(get_now() - timer->start), timer->other );
	if (timer->other >= 0) {
		timer_start( timer, timer->other );
	} else {
		wipe_wakeup( &timer->timer );
		wipe_wakeup( &timer->morph_timer );
		free( timer );
	}
}

static void
morph_timed_out( void *aux )
{
	tst_t *timer = (tst_t *)aux;

	printf( "morphing timer %d after %d\n",
	        timer->id, (int)(get_now() - timer->start) );
	timer_start( timer, timer->morph_to );
}

static int nextid;

int
main( int argc, char **argv )
{
	int i;

	init_timers();
	for (i = 1; i < argc; i++) {
		char *val = argv[i];
		tst_t *timer = nfmalloc( sizeof(*timer) );
		init_wakeup( &timer->timer, timed_out, timer );
		init_wakeup( &timer->morph_timer, morph_timed_out, timer );
		timer->id = ++nextid;
		timer->first = strtol( val, &val, 0 );
		if (*val == '@') {
			timer->other = timer->first;
			timer->first = strtol( ++val, &val, 0 );
		} else {
			timer->other = -1;
		}
		if (*val == ':') {
			timer->morph_to = strtol( ++val, &val, 0 );
			if (*val != '@')
				goto fail;
			timer->morph_at = strtol( ++val, &val, 0 );
		} else {
			timer->morph_at = -1;
		}
		if (*val) {
		  fail:
			fprintf( stderr, "Fatal: syntax error in %s, use <timeout>[@<delay>][:<newtimeout>@<delay>]\n", argv[i] );
			return 1;
		}
		timer_start( timer, timer->first );
		if (timer->morph_at >= 0) {
			printf( "timer %d, should morph after %d\n", timer->id, timer->morph_at );
			conf_wakeup( &timer->morph_timer, timer->morph_at );
		}
	}

	main_loop();
	return 0;
}
