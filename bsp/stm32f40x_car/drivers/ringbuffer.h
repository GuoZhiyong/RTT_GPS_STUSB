#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_


typedef struct _ringbuffer
{
	unsigned int rd;
	unsigned int wr;
	unsigned int total;
	unsigned int count;
	unsigned char *pdata; //指向保存数据的指针地址
}RingBuffer;






void ringbuffer_init(RingBuffer *rb,unsigned int size,unsigned char *data);
int ringbuffer_get(RingBuffer *rb,unsigned char *data);
int ringbuffer_put(RingBuffer *rb,unsigned char data);
int ringbuffer_count(RingBuffer *rb);
int ringbuffer_put_data(RingBuffer *rb,unsigned int size,unsigned char *pdata);








#endif


