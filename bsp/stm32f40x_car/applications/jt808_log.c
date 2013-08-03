#include <rtthread.h>
#include "jt808.h"
#include "sst25.h"

/*
工作异常的记录
libcpu/cpuport.c
	void rt_hw_hard_fault_exception(struct exception_stack_frame * exception_stack)

rtdebug.h中 
	RT_ASSERT 的信息


实际上是rt_kprintf的重定向
kservice.c中

_console_device==RT_NULL


*/

void save_log(void)
{



}




