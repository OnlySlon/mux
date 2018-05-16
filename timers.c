#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "mux.h"
#ifndef TEST


#endif

#ifdef TEST
	#define TCP_TMR_INTERVAL 1000000
	#define tdx_gettimeofday gettimeofday
#endif 

/*
 * All events have a name, an expiration time and a function to be called
 * at this time. The queue is always sorted so the next event to happen is
 * first in the queue.
 */
struct event
{
	TAILQ_ENTRY (event) next;
	struct timeval   expire;
	char            *name;
	void            (*fun) (void *);
	void             *arg;
	int               tid;
};

static TAILQ_HEAD (event_head, event) events;




clock_time_t clock_time(void)
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Initialize timer event queue. */
void timers_init(void)
{
	TAILQ_INIT(&events);
}

/*
 * Return the number of seconds until the next event happens. Used for
 * the select() call in the main loop.
 */
void timer_next_event(struct timeval *tv)
{
	struct timeval   now;
	struct event    *e = TAILQ_FIRST(&events);

	if (e)
	{
		gettimeofday(&now, 0);
		if (timercmp(&now, &e->expire, >=))
			timerclear(tv);
		else
			timersub(&e->expire, &now, tv);
//		printf("Zometimerz\n");
	}
	else
	{
		tv->tv_sec  = 0;	/* "Best guess". */
		tv->tv_usec = 250 * 1000;	 // TCP_TIMER 250ms.
//		printf("Best guess tv_usec=%u\n", tv->tv_usec);
	}
}

/*
 * Whenever select() times out, we have an event that should happen and this
 * routine gets called. Handle and remove all pending events.
 */
void timer_run(void)
{
	struct timeval   now;
	struct event    *e;

	gettimeofday(&now, 0);
	for (e = TAILQ_FIRST(&events); e && timercmp(&now, &e->expire, >=); e = TAILQ_FIRST(&events))
	{
		TAILQ_REMOVE(&events, e, next);
//		printf("timer_run: event \"%s\"\n", e->name ? e->name : "<unknown>");
//		clog(gl_cfg->info, CMARK, DBG_SYSTEM, "F:%s: TIMER RUN '%s' func: %p  now=%llu expire=%llu", __FUNCTION__, e->name, e->fun, now.tv_sec , e->expire.tv_sec);
		if (e->fun)	(*e->fun)(e->arg);
		if (e->name)
			free(e->name);
		free(e);
	}
}

int timer_add_ms(char *name, unsigned int ms, void (*function)(void *), void *arg)
{
	struct timeval then;
	// // 1000 - 1ms
	then.tv_sec  = ms / 1000;
//     ms / 1000;
//	 clog(gl_cfg->info, CMARK, DBG_SYSTEM, "F:%s: TIMER ADD '%s' Msec:%u", __FUNCTION__, name, ms);
	then.tv_usec = (ms%1000) * 1000;
	timer_add(0, name, then, function, arg);
	return 0;
}

// use TaskId (tid) - can replace existing timer
int timer_add_ms_unique(int tid, char *name, unsigned int ms, void (*function)(void *), void *arg)
{
	struct timeval then;
	// // 1000 - 1ms
	then.tv_sec  = ms / 1000;
//     ms / 1000;
//	 clog(gl_cfg->info, CMARK, DBG_SYSTEM, "F:%s: TIMER ADD '%s' Msec:%u", __FUNCTION__, name, ms);
	then.tv_usec = (ms%1000) * 1000;
	timer_add(tid, name, then, function, arg);
	return 0;
}


/* Add a new event. */
int timer_add(int tid, char *name, struct timeval when, void (*function)(void *), void *arg)
{
	struct timeval   now, tmp;
	struct event    *e, *new;

	new = (struct event *)calloc(1, sizeof *new);
	if (!new)
	{
//		log_err("timer_add: calloc (1, %u) failed", sizeof *new);
		return -1;
	}
//	printf("timer_add:%s ms=%u\n", name, (when.tv_usec/1000));
	new->name = strdup(name); /* We handle failures here. */
	new->fun = function;
	new->arg = arg;
	new->tid = tid;

	memset(&tmp, 0, sizeof tmp);
	tmp.tv_sec   = when.tv_sec;
	tmp.tv_usec  = when.tv_usec;
	gettimeofday(&now, 0);
	timeradd(&now, &tmp, &new->expire);

//	clog(gl_cfg->info, CMARK, DBG_SYSTEM, "F:%s: TIMER ADD '%s' %u ms.", __FUNCTION__, name, (when.tv_sec * 1000 + when.tv_usec / 1000));

//	printf("NOW=%llu SET EXPIRE=%llu\n", now.tv_sec, new->expire.tv_sec  );

//	printf("timer_add: new event \"%s\" (expiring in %us)\n",
//	    name ? name : "<unknown>", when);

	/* Insert the new event in the queue so it's always sorted. */
	if (tid)
	{
		for (e = TAILQ_FIRST(&events); e; e = TAILQ_NEXT(e, next))
		{
			if (e->tid == tid)
			{
				TAILQ_REMOVE(&events, e, next);
				if (e->name) free(e->name);
			}
		}

	}


	for (e = TAILQ_FIRST(&events); e; e = TAILQ_NEXT(e, next))
	{
		if (timercmp(&new->expire, &e->expire, >=))
			continue;
		TAILQ_INSERT_BEFORE(e, new, next);
		return 0;
	}
	TAILQ_INSERT_TAIL(&events, new, next);
	return 0;
}





void timer_set(struct timer *t, clock_time_t interval)
{
	t->interval = interval;
	t->start = clock_time();
}

void timer_reset(struct timer *t)
{
	t->start += t->interval;
}

void timer_restart(struct timer *t)
{
	t->start = clock_time();
}

int timer_expired(struct timer *t)
{
	return(clock_time_t)(clock_time() - t->start) >= (clock_time_t)t->interval;
}

int timer_lost(struct timer *t)
{
	if (!((clock_time_t)(clock_time() - t->start) >= (clock_time_t)t->interval))
	{
		return(u_int32_t) t->interval - (clock_time() - t->start);
	}
	else
		return 0xFFFF;
}

int timer_elapsed(struct timer *t)
{
	if ((clock_time_t)(clock_time() - t->start) < (clock_time_t) t->interval)
	{
		// timer not expired
		return(int) (clock_time() - t->start);
	}
	else
		return -1;
}



#ifdef TEST
void main()
{
	timers_init();
	struct timeval ev;

	ev.tv_sec = 0;
	ev.tv_usec = 500000;
	timer_add(0, "timer_500ms", ev, NULL, NULL);


	ev.tv_sec = 5;
	ev.tv_usec = 500000;
	timer_add(2, "timer_5sec_1", ev, NULL, NULL);

	ev.tv_sec = 5;
	ev.tv_usec = 500000;
	timer_add(2, "timer_5sec_2", ev, NULL, NULL);


	ev.tv_sec = 3;
	ev.tv_usec = 500000;
	timer_add(2, "timer_5sec_2", ev, NULL, NULL);


	ev.tv_sec = 10;
	ev.tv_usec = 1000;
	timer_add(0,"timer_10sec", ev, NULL, NULL);

	struct timeval ne;

	for ( ; ; )
	{
		timer_next_event(&ne);
		printf("NE: %u sec %u usec\n", ne.tv_sec, ne.tv_usec);
		usleep(1000000);
		timer_run();
	}

}
#endif 