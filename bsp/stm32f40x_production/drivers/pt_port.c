#include <rtthread.h>

#include "pt_port.h"

/*��ȡ��ǰʱ��*/
int pt_clock_time( void )
{
	return rt_tick_get( );
}

/*����Ƿ�ʱ*/
int pt_timer_expired( struct pt_timer *t )
{
	return (int)( pt_clock_time( ) - t->start ) >= (int)t->interval;
}

/*�趨һ���򵥵Ķ�ʱ��*/
void pt_timer_set( struct pt_timer * t, int interval )
{
	t->interval = interval;
	t->start	= pt_clock_time( );
}

