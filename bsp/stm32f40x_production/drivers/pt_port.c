#include <rtthread.h>

#include "pt_port.h"

/*获取当前时刻*/
int pt_clock_time( void )
{
	return rt_tick_get( );
}

/*检查是否超时*/
int pt_timer_expired( struct pt_timer *t )
{
	return (int)( pt_clock_time( ) - t->start ) >= (int)t->interval;
}

/*设定一个简单的定时器*/
void pt_timer_set( struct pt_timer * t, int interval )
{
	t->interval = interval;
	t->start	= pt_clock_time( );
}

