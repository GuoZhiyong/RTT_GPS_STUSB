#ifndef _PT_PORT_H_
#define _PT_PORT_H_


#include <rtthread.h>

/*�򵥵�ʱ��������*/
struct pt_timer
{
	rt_tick_t	start;
	rt_tick_t	interval;
};

int pt_clock_time( void );
int pt_timer_expired( struct pt_timer *t );
void pt_timer_set( struct pt_timer * t, int interval );

#endif


