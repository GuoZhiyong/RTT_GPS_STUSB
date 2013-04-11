#ifndef SED1520_H_
#define SED1520_H_

// define to disable busy-delays (useful for simulation)
//// #define LCD_SIM (1)
// define to enable additional debugging functions
//// #define LCD_DEBUG (1)

#include <stdint.h>

typedef struct IMG_DEF 
 { unsigned char width_in_pixels;      /* Image width */
   unsigned char height_in_pixels;     /* Image height*/
   const unsigned char *char_table;    /* Image table start address in memory  */
 } IMG_DEF;


#define DECL_BMP(width,height,imgdata)	IMG_DEF bmp_##imgdata={width,height,imgdata}


/* draw modes */
#define LCD_MODE_CLEAR     0
#define LCD_MODE_SET       1
#define LCD_MODE_XOR       2
#define LCD_MODE_INVERT		3

/* command function equates for SED1520 LCD Display Controller */
#define LCD_DISP_OFF       0xAE	/* turn LCD panel OFF */
#define LCD_DISP_ON        0xAF	/* turn LCD panel ON */
#define LCD_SET_LINE       0xC0	/* set line for COM0 (4 lsbs = ST3:ST2:ST1:ST0) */
#define LCD_SET_PAGE       0xB8	/* set page address (2 lsbs = P1:P0) */
#define LCD_SET_COL        0x00	/* set column address (6 lsbs = Y4:Y4:Y3:Y2:Y1:Y0) */
#define LCD_SET_ADC_NOR    0xA0	/* ADC set for normal direction */
#define LCD_SET_ADC_REV    0xA1	/* ADC set for reverse direction */
#define LCD_STATIC_OFF     0xA4	/* normal drive */
#define LCD_STATIC_ON      0xA5	/* static drive (power save) */
#define LCD_DUTY_16        0xA8	/* driving duty 1/16 */
#define LCD_DUTY_32        0xA9	/* driving duty 1/32 */
#define LCD_SET_MODIFY     0xE0	/* start read-modify-write mode */
#define LCD_CLR_MODIFY     0xEE	/* end read-modify-write mode */
#define LCD_RESET          0xE2	/* soft reset command */

/* LCD screen and bitmap image array consants */
#define LCD_X_BYTES		122
#define LCD_Y_BYTES		4
#define SCRN_LEFT		0
#define SCRN_TOP		0
#define SCRN_RIGHT		121
#define SCRN_BOTTOM		31

/* SED1520 is used with reverse direction (ADC_REV). 
   This value is the address of the leftmost column: */
#define LCD_STARTCOL_REVERSE	19



void lcd_init(void);
void lcd_fill( const unsigned char pattern );
void lcd_update(const unsigned char top, const unsigned char bottom);
void lcd_asc0608( char left, char top, char *p,const char mode );
void lcd_bitmap(const uint8_t left, const uint8_t top, const struct IMG_DEF *img_ptr, const uint8_t mode);
void lcd_clear( const unsigned char top, const unsigned char bottom );

void lcd_hline(uint8_t from,uint8_t to ,uint8_t line);
void lcd_vline(uint8_t col,uint8_t from,uint8_t to);
void lcd_fill_1(uint8_t left,uint8_t top,uint8_t right,uint8_t bottom,uint8_t pattern);



#endif
