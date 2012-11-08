#include "ringbuffer.h"


/*
初始化一个ringbuffer,没有使用动态内存
*/
void ringbuffer_init(RingBuffer *rb,unsigned int size,unsigned char *data)
{
	rb->count=0;
	rb->total=size;
	rb->rd=0;
	rb->wr=0;
	rb->pdata=data;
}


/*
从ringbuffer中获取一个数据，
如果成功 返回0
    失败 返回-1
*/

int ringbuffer_get(RingBuffer *rb,unsigned char *data)
{
	if (rb->count==0) return -1;
	*data=*(rb->pdata+rb->rd);
	rb->rd++;
	if(rb->rd>=rb->total) rb->rd=0;
	rb->count--;
	return 0;
}

/*
向ringbuffer中写入一个数据，
如果成功 返回0
    失败 返回-1
*/

int ringbuffer_put(RingBuffer *rb,unsigned char data)
{
	if (rb->count>=rb->total) return -1;
	*(rb->pdata+rb->wr)=data;
	rb->wr++;
	if(rb->wr>=rb->total) rb->wr=0;
	rb->count++;
	return 0;
}

/*
向ringbuffer中批量写入数据
如果成功 返回0
    失败 返回-1
*/

int ringbuffer_put_data(RingBuffer *rb,unsigned int size,unsigned char *pdata)
{
	unsigned int count=size;
	unsigned char *p=pdata;
	if (rb->total-rb->count<size) return -1; //没有足够的空间
	while(count)
	{
		*(rb->pdata+rb->wr)=*pdata++;
		rb->wr++;
		if(rb->wr>=rb->total) rb->wr=0;
		rb->count++;
		count--;
	}
	return 0;
}



/*
返回ringbuffer中数据个数
*/

int ringbuffer_count(RingBuffer* rb)
{
	return rb->count;
}


