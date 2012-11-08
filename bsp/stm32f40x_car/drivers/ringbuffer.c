#include "ringbuffer.h"


/*
��ʼ��һ��ringbuffer,û��ʹ�ö�̬�ڴ�
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
��ringbuffer�л�ȡһ�����ݣ�
����ɹ� ����0
    ʧ�� ����-1
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
��ringbuffer��д��һ�����ݣ�
����ɹ� ����0
    ʧ�� ����-1
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
��ringbuffer������д������
����ɹ� ����0
    ʧ�� ����-1
*/

int ringbuffer_put_data(RingBuffer *rb,unsigned int size,unsigned char *pdata)
{
	unsigned int count=size;
	unsigned char *p=pdata;
	if (rb->total-rb->count<size) return -1; //û���㹻�Ŀռ�
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
����ringbuffer�����ݸ���
*/

int ringbuffer_count(RingBuffer* rb)
{
	return rb->count;
}


