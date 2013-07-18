#include "jt808_gps.h"



/*
   $GNRMC,084251.00,A,3909.263701,N,11712.285640,E,3.5,11.1,180713,,,A*7A
   原来为70字节

   压缩后 28byte

 */
typedef struct __packed
{
	uint32_t	id;
	MYTIME		datetime;
	uint8_t		fixed; /*A V*/
	float		lati;
	uint8_t		NS;
	float		longi;
	uint8_t		EW;
	float		spd;
	float		cog;
	uint8_t		mode;
}PACKED_RMC;


/*
   变长度
   $GPGSV,2,1,5,08,40,221,39,20,28,141,35,09,37,238,35,07,18,194,40*46
   $GPGSV,2,2,5,01,69,058,26*7B
   $BDGSV,2,1,6,06,26,165,37,10,25,215,40,01,38,146,42,03,45,190,42*5E
   $BDGSV,2,2,6,04,26,124,37,02,29,229,31*5F
   最长67，最短28
   压缩后 25
 */
typedef struct __packed
{
	uint32_t	id;
	uint8_t		NoMsg;
	uint8_t		MsgNo;
	uint8_t		NoSv;
	uint8_t		NS;
	uint8_t		count; /*变长度的1--4个，记录一下*/
	uint8_t		sv_1;
	uint8_t		elv_1;
	uint16_t	az_1;
	uint8_t		cno_1;
	uint8_t		sv_2;
	uint8_t		elv_2;
	uint16_t	az_2;
	uint8_t		cno_2;
	uint8_t		sv_3;
	uint8_t		elv_3;
	uint16_t	az_3;
	uint8_t		cno_3;
	uint8_t		sv_4;
	uint8_t		elv_4;
	uint16_t	az_4;
	uint8_t		cno_4;
}PACKED_GSV;


/*
   $GNGGA,084254.00,3909.263746,N,11712.285632,E,1,11,2.0,15.8,M,-2.8,M,,,3.3*5B

   77字节
 */
typedef struct __packed
{
	uint32_t	id;
	MYTIME		time;
	float		lati;
	uint8_t		NS;
	float		longi;
	uint8_t		EW;
	uint8_t		FS;
	uint8_t		NoSV;
	float		HDOP;
	float		msl;
	float		Altref;
	float		spd;
	float		cog;
	uint8_t		mode;
}PACKED_GGA;


/*
   $GNGLL,3909.263746,N,11712.285632,E,084254.00,A,0*04
 */
typedef struct __packed
{
	uint32_t	id;
	MYTIME		datetime;
	uint8_t		fixed; /*A V*/
	float		lati;
	uint8_t		NS;
	float		longi;
	uint8_t		EW;
	float		spd;
	float		cog;
	uint8_t		mode;
}PACKED_GLL;


/*
   $GPGSA,A,3,08,20,09,07,01,,,,,,,,3.8,2.0,3.3,2.8*36
   $BDGSA,A,3,06,10,01,03,04,02,,,,,,,3.8,2.0,3.3,2.8*21
 */
typedef struct __packed
{
	uint32_t	id;
	uint8_t		Smode; /*A M*/
	uint8_t 	FS; /*fix status*/
	uint8_t		sv[12]; /*如果没有,填写0,还原时是相连的逗号*/
	uint8_t		PDOP;
	uint8_t		PDOP_frac;
	uint8_t		HDOP;
	uint8_t		HDOP_frac;
	uint8_t		VDOP;
	uint8_t		VDOP_frac;
}   PACKED_GSA;


/*
   $GNTXT,01,01,01,ANTENNA OK*2B
 */
typedef struct __packed
{
	uint32_t	id;
	uint8_t		mode;  /*代表固定的含义*/
} PACKED_TXT;



/*使用除$符号的后4个字节作为id*/
#define BDGSA	0x42444753

void pack_rmc(char *pinfo)
{



}


void pack_gsv(char *pinfo)
{



}

void pack_gga(char *pinfo)
{



}


void pack_gll(char *pinfo)
{



}


void pack_gsa(char *pinfo)
{



}

void pack_txt(char *pinfo)
{



}




