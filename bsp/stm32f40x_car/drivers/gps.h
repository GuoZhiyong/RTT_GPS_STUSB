#ifndef _GPS_H_
#define _GPS_H_

#define CTL_GPS_BAUD	0x30
#define CTL_GPS_OUTMODE	0x31	/*gps信息的输出模式*/


#define GPS_OUTMODE_TRIGGER	0x01	/*秒触发输出*/
#define GPS_OUTMODE_GPRMC	0x02
#define GPS_OUTMODE_GPGSV	0x04
#define GPS_OUTMODE_BD		0x08

#define GPS_OUTMODE_ALL		0xFFFFFFFF	/*全数据输出*/


#endif

