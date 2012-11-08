#ifndef _MMA845X_H_

#define _MMA845X_H_


void mma8451_init(void);
void mma8451_proc(void);
int mma8451_rx(unsigned char * pdata,unsigned int len);

#endif  /* _MMA845X_H_ */




