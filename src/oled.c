/******************************************************************************
 作 者 : 宋宝善
 版本号 : 1.0
 生成日期: 2021年3月1日
 概	  述:
 1. oled液晶操作相关函数, 显示汉字及ASCII码
 2. OLED的显存存放格式如下.
 [page0]0 1 2 3 ... 127
 [page1]0 1 2 3 ... 127
 [page2]0 1 2 3 ... 127
 [page3]0 1 2 3 ... 127
 [page4]0 1 2 3 ... 127
 [page5]0 1 2 3 ... 127
 [page6]0 1 2 3 ... 127
 [page7]0 1 2 3 ... 127

 修改日志:
 1.日期: 2021年3月1日
 修改内容: 创建文件
 ******************************************************************************/

#include "oled.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

typedef struct oledPageLayout {
    u8 curRow;
    u8 curCol;
    u8 curPage;
} oledPageLayout_s;

/**
 * 存储字符串在屏幕上的位置
 * 信息
 */
typedef struct oledStringPosition {
    char *pString;                  //字符串指针
    int charIdx;                    //要查找的字符索引
    int byteIdx;                    //要查找的字符的字节索引
    u8 curRow;                      //字符所在行
    u8 curCol;                      //字符所在列
} oledStringPosition_s;

static u8 s_OLED_GRAM[PAGE_COUNT][MAX_COLUMN] = { 0 };
static u8 s_PIXEL_GRAM[MAX_ROW][MAX_COLUMN] = { 0 };
static oledPageLayout_s s_oled_layout = { 0 };
static char s_OLED_upperHexLetter[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
static const unsigned char F8X16[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // space
        0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x30, 0x00, 0x00, 0x00, //! 1
        0x00, 0x10, 0x0C, 0x06, 0x10, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //" 2
        0x40, 0xC0, 0x78, 0x40, 0xC0, 0x78, 0x40, 0x00, 0x04, 0x3F, 0x04, 0x04, 0x3F, 0x04, 0x04, 0x00, //# 3
        0x00, 0x70, 0x88, 0xFC, 0x08, 0x30, 0x00, 0x00, 0x00, 0x18, 0x20, 0xFF, 0x21, 0x1E, 0x00, 0x00, //$ 4
        0xF0, 0x08, 0xF0, 0x00, 0xE0, 0x18, 0x00, 0x00, 0x00, 0x21, 0x1C, 0x03, 0x1E, 0x21, 0x1E, 0x00, //% 5
        0x00, 0xF0, 0x08, 0x88, 0x70, 0x00, 0x00, 0x00, 0x1E, 0x21, 0x23, 0x24, 0x19, 0x27, 0x21, 0x10, //& 6
        0x10, 0x16, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //' 7
        0x00, 0x00, 0x00, 0xE0, 0x18, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x07, 0x18, 0x20, 0x40, 0x00, //( 8
        0x00, 0x02, 0x04, 0x18, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x18, 0x07, 0x00, 0x00, 0x00, //) 9
        0x40, 0x40, 0x80, 0xF0, 0x80, 0x40, 0x40, 0x00, 0x02, 0x02, 0x01, 0x0F, 0x01, 0x02, 0x02, 0x00, //* 10
        0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x1F, 0x01, 0x01, 0x01, 0x00, //+ 11
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xB0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, //, 12
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //- 13
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, //. 14
        0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x18, 0x04, 0x00, 0x60, 0x18, 0x06, 0x01, 0x00, 0x00, 0x00, /// 15
        0x00, 0xE0, 0x10, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x00, 0x0F, 0x10, 0x20, 0x20, 0x10, 0x0F, 0x00, //0 16
        0x00, 0x10, 0x10, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00, //1 17
        0x00, 0x70, 0x08, 0x08, 0x08, 0x88, 0x70, 0x00, 0x00, 0x30, 0x28, 0x24, 0x22, 0x21, 0x30, 0x00, //2 18
        0x00, 0x30, 0x08, 0x88, 0x88, 0x48, 0x30, 0x00, 0x00, 0x18, 0x20, 0x20, 0x20, 0x11, 0x0E, 0x00, //3 19
        0x00, 0x00, 0xC0, 0x20, 0x10, 0xF8, 0x00, 0x00, 0x00, 0x07, 0x04, 0x24, 0x24, 0x3F, 0x24, 0x00, //4 20
        0x00, 0xF8, 0x08, 0x88, 0x88, 0x08, 0x08, 0x00, 0x00, 0x19, 0x21, 0x20, 0x20, 0x11, 0x0E, 0x00, //5 21
        0x00, 0xE0, 0x10, 0x88, 0x88, 0x18, 0x00, 0x00, 0x00, 0x0F, 0x11, 0x20, 0x20, 0x11, 0x0E, 0x00, //6 22
        0x00, 0x38, 0x08, 0x08, 0xC8, 0x38, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, //7 23
        0x00, 0x70, 0x88, 0x08, 0x08, 0x88, 0x70, 0x00, 0x00, 0x1C, 0x22, 0x21, 0x21, 0x22, 0x1C, 0x00, //8 24
        0x00, 0xE0, 0x10, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x00, 0x00, 0x31, 0x22, 0x22, 0x11, 0x0F, 0x00, //9 25
        0x00, 0x00, 0x00, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, //: 26
        0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x00, 0x00, 0x00, 0x00, //; 27
        0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, //< 28
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, //= 29
        0x00, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, //> 30
        0x00, 0x70, 0x48, 0x08, 0x08, 0x08, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x36, 0x01, 0x00, 0x00, //? 31
        0xC0, 0x30, 0xC8, 0x28, 0xE8, 0x10, 0xE0, 0x00, 0x07, 0x18, 0x27, 0x24, 0x23, 0x14, 0x0B, 0x00, //@ 32
        0x00, 0x00, 0xC0, 0x38, 0xE0, 0x00, 0x00, 0x00, 0x20, 0x3C, 0x23, 0x02, 0x02, 0x27, 0x38, 0x20, //A 33
        0x08, 0xF8, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x20, 0x11, 0x0E, 0x00, //B 34
        0xC0, 0x30, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00, 0x07, 0x18, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00, //C 35
        0x08, 0xF8, 0x08, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x20, 0x10, 0x0F, 0x00, //D 36
        0x08, 0xF8, 0x88, 0x88, 0xE8, 0x08, 0x10, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x23, 0x20, 0x18, 0x00, //E 37
        0x08, 0xF8, 0x88, 0x88, 0xE8, 0x08, 0x10, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x03, 0x00, 0x00, 0x00, //F 38
        0xC0, 0x30, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x07, 0x18, 0x20, 0x20, 0x22, 0x1E, 0x02, 0x00, //G 39
        0x08, 0xF8, 0x08, 0x00, 0x00, 0x08, 0xF8, 0x08, 0x20, 0x3F, 0x21, 0x01, 0x01, 0x21, 0x3F, 0x20, //H 40
        0x00, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00, //I 41
        0x00, 0x00, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x00, 0xC0, 0x80, 0x80, 0x80, 0x7F, 0x00, 0x00, 0x00, //J 42
        0x08, 0xF8, 0x88, 0xC0, 0x28, 0x18, 0x08, 0x00, 0x20, 0x3F, 0x20, 0x01, 0x26, 0x38, 0x20, 0x00, //K 43
        0x08, 0xF8, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x20, 0x20, 0x30, 0x00, //L 44
        0x08, 0xF8, 0xF8, 0x00, 0xF8, 0xF8, 0x08, 0x00, 0x20, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x20, 0x00, //M 45
        0x08, 0xF8, 0x30, 0xC0, 0x00, 0x08, 0xF8, 0x08, 0x20, 0x3F, 0x20, 0x00, 0x07, 0x18, 0x3F, 0x00, //N 46
        0xE0, 0x10, 0x08, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x0F, 0x10, 0x20, 0x20, 0x20, 0x10, 0x0F, 0x00, //O 47
        0x08, 0xF8, 0x08, 0x08, 0x08, 0x08, 0xF0, 0x00, 0x20, 0x3F, 0x21, 0x01, 0x01, 0x01, 0x00, 0x00, //P 48
        0xE0, 0x10, 0x08, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x0F, 0x18, 0x24, 0x24, 0x38, 0x50, 0x4F, 0x00, //Q 49
        0x08, 0xF8, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x03, 0x0C, 0x30, 0x20, //R 50
        0x00, 0x70, 0x88, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x38, 0x20, 0x21, 0x21, 0x22, 0x1C, 0x00, //S 51
        0x18, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x18, 0x00, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x00, 0x00, //T 52
        0x08, 0xF8, 0x08, 0x00, 0x00, 0x08, 0xF8, 0x08, 0x00, 0x1F, 0x20, 0x20, 0x20, 0x20, 0x1F, 0x00, //U 53
        0x08, 0x78, 0x88, 0x00, 0x00, 0xC8, 0x38, 0x08, 0x00, 0x00, 0x07, 0x38, 0x0E, 0x01, 0x00, 0x00, //V 54
        0xF8, 0x08, 0x00, 0xF8, 0x00, 0x08, 0xF8, 0x00, 0x03, 0x3C, 0x07, 0x00, 0x07, 0x3C, 0x03, 0x00, //W 55
        0x08, 0x18, 0x68, 0x80, 0x80, 0x68, 0x18, 0x08, 0x20, 0x30, 0x2C, 0x03, 0x03, 0x2C, 0x30, 0x20, //X 56
        0x08, 0x38, 0xC8, 0x00, 0xC8, 0x38, 0x08, 0x00, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x00, 0x00, //Y 57
        0x10, 0x08, 0x08, 0x08, 0xC8, 0x38, 0x08, 0x00, 0x20, 0x38, 0x26, 0x21, 0x20, 0x20, 0x18, 0x00, //Z 58
        0x00, 0x00, 0x00, 0xFE, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x40, 0x40, 0x40, 0x00, //[ 59
        0x00, 0x0C, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x38, 0xC0, 0x00, //\ 60
        0x00, 0x02, 0x02, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x7F, 0x00, 0x00, 0x00, //] 61
        0x00, 0x00, 0x04, 0x02, 0x02, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //^ 62
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, //_ 63
        0x00, 0x02, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //` 64
        0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x19, 0x24, 0x22, 0x22, 0x22, 0x3F, 0x20, //a 65
        0x08, 0xF8, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x11, 0x20, 0x20, 0x11, 0x0E, 0x00, //b 66
        0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x0E, 0x11, 0x20, 0x20, 0x20, 0x11, 0x00, //c 67
        0x00, 0x00, 0x00, 0x80, 0x80, 0x88, 0xF8, 0x00, 0x00, 0x0E, 0x11, 0x20, 0x20, 0x10, 0x3F, 0x20, //d 68
        0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x1F, 0x22, 0x22, 0x22, 0x22, 0x13, 0x00, //e 69
        0x00, 0x80, 0x80, 0xF0, 0x88, 0x88, 0x88, 0x18, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00, //f 70
        0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x6B, 0x94, 0x94, 0x94, 0x93, 0x60, 0x00, //g 71
        0x08, 0xF8, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x20, 0x3F, 0x21, 0x00, 0x00, 0x20, 0x3F, 0x20, //h 72
        0x00, 0x80, 0x98, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00, //i 73
        0x00, 0x00, 0x00, 0x80, 0x98, 0x98, 0x00, 0x00, 0x00, 0xC0, 0x80, 0x80, 0x80, 0x7F, 0x00, 0x00, //j 74
        0x08, 0xF8, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x20, 0x3F, 0x24, 0x02, 0x2D, 0x30, 0x20, 0x00, //k 75
        0x00, 0x08, 0x08, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00, //l 76
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x3F, 0x20, 0x00, 0x3F, //m 77
        0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x20, 0x3F, 0x21, 0x00, 0x00, 0x20, 0x3F, 0x20, //n 78
        0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x1F, 0x20, 0x20, 0x20, 0x20, 0x1F, 0x00, //o 79
        0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xA1, 0x20, 0x20, 0x11, 0x0E, 0x00, //p 80
        0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x0E, 0x11, 0x20, 0x20, 0xA0, 0xFF, 0x80, //q 81
        0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x20, 0x20, 0x3F, 0x21, 0x20, 0x00, 0x01, 0x00, //r 82
        0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x33, 0x24, 0x24, 0x24, 0x24, 0x19, 0x00, //s 83
        0x00, 0x80, 0x80, 0xE0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x20, 0x20, 0x00, 0x00, //t 84
        0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x1F, 0x20, 0x20, 0x20, 0x10, 0x3F, 0x20, //u 85
        0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x01, 0x0E, 0x30, 0x08, 0x06, 0x01, 0x00, //v 86
        0x80, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x80, 0x0F, 0x30, 0x0C, 0x03, 0x0C, 0x30, 0x0F, 0x00, //w 87
        0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x20, 0x31, 0x2E, 0x0E, 0x31, 0x20, 0x00, //x 88
        0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x81, 0x8E, 0x70, 0x18, 0x06, 0x01, 0x00, //y 89
        0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x21, 0x30, 0x2C, 0x22, 0x21, 0x30, 0x00, //z 90
        0x00, 0x00, 0x00, 0x00, 0x80, 0x7C, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x40, 0x40, //{ 91
        0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, //| 92
        0x00, 0x02, 0x02, 0x7C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00, //} 93
        0x00, 0x06, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //~ 94
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*"",95*/
};

static sqlite3 *s_db;

int openFontDB(char *dbfile)
{
    return sqlite3_open(dbfile, &s_db) == SQLITE_OK ? 1 : -1;
}

int closeFontDB(void)
{
    return (sqlite3_close(s_db) == SQLITE_OK) ? 1 : -1;
}

static int getResult(char *sql, char *buf)
{
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(s_db, sql, strlen(sql), &stmt, NULL);
    int result = sqlite3_step(stmt);
    int len = 0;
    if (result == SQLITE_ROW)
    {
        const char *pdata = sqlite3_column_blob(stmt, 0);
        len = sqlite3_column_bytes(stmt, 0);
        memcpy(buf, pdata, len);
    }

    sqlite3_finalize(stmt);

    return len;
}

int getFontUTF8(int unicode, char *buf)
{
    char sql[128];

    snprintf(sql, 127, "select font from t_unicode_gbk where unicode=%d;", unicode);

    return getResult(sql, buf);
}

int getFontGBK(int gbk, char *buf)
{
    char sql[128];

    snprintf(sql, 127, "select font from t_unicode_gbk where gbk=%d;", gbk);

    return getResult(sql, buf);
}

void OLED_expand_byte(u8 dat, int row, int col)
{
	u8 tmp = 0;
	int i = 0;
	char c = 0;
	for (i = 0; i < PAGE_HEIGHT; i++)
	{
		tmp = dat & (1 << i);
		c = (tmp ? '#' : ' ');
		s_PIXEL_GRAM[row * PAGE_HEIGHT + i][col] = c;
	}
}

void OLED_print_canvas()
{
	int i = 0;
	int j = 0;
	for (i = 0; i < MAX_ROW; i++)
	{
		for (j = 0; j < MAX_COLUMN; j++)
		{
			printf("%c", s_PIXEL_GRAM[i][j]);
		}

		printf("\n");
	}
}

//更新显存到OLED
void OLED_Refresh()
{
	u8 col = 0;
	u8 row = 0;

	for (row = 0; row < PAGE_COUNT; row++)
	{
		for (col = 0; col < MAX_COLUMN; col++)
		{
			OLED_expand_byte(s_OLED_GRAM[row][col], row, col);
		}
	}

	OLED_print_canvas();
}

//反显函数
void OLED_ColorTurn()
{
	int row = 0;
	int col = 0;
	for (row = 0; row < PAGE_COUNT; row++)
	{
		for (col = 0; col < MAX_COLUMN; col++)
		{
			s_OLED_GRAM[row][col] = ~s_OLED_GRAM[row][col];
		}
	}
}

//画点
//x:0~127
//y:0~63
//t:1 填充 0,清空
void OLED_DrawPoint(u8 col, u8 y, u8 t)
{
	u8 row, m, n;
	row = y / 8; //像素位于哪一行，1行=8个纵向像素
	m = y % 8; //像素位于一列中的哪一行，从上到下开始数
	n = 1 << m; //像素的值
	if (t) //如果是点亮，则将这一像素对应的位, 置1
	{
		s_OLED_GRAM[row][col] |= n;
	}
	else   //如果是熄灭，则将这一像素对应的位, 置0
	{
		s_OLED_GRAM[row][col] &= (~(1 << m));
	}
}

//画线
//x1,y1:起点坐标
//x2,y2:结束坐标
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1; //计算坐标增量
	delta_y = y2 - y1;
	uRow = x1; //画线起点坐标
	uCol = y1;
	if (delta_x > 0)
		incx = 1; //设置单步方向
	else if (delta_x == 0)
		incx = 0; //垂直线
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}

	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; //水平线
	else
	{
		incy = -1;
		delta_y = -delta_x;
	}
	if (delta_x > delta_y)
		distance = delta_x; //选取基本增量坐标轴
	else
		distance = delta_y;
	for (t = 0; t < distance + 1; t++)
	{
		OLED_DrawPoint(uRow, uCol, mode); //画点
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}

//x,y:圆心坐标
//r:圆的半径
void OLED_DrawCircle(u8 x, u8 y, u8 r)
{
	int a, b, num;
	a = 0;
	b = r;
	while (2 * b * b >= r * r)
	{
		OLED_DrawPoint(x + a, y - b, 1);
		OLED_DrawPoint(x - a, y - b, 1);
		OLED_DrawPoint(x - a, y + b, 1);
		OLED_DrawPoint(x + a, y + b, 1);

		OLED_DrawPoint(x + b, y + a, 1);
		OLED_DrawPoint(x + b, y - a, 1);
		OLED_DrawPoint(x - b, y - a, 1);
		OLED_DrawPoint(x - b, y + a, 1);

		a++;
		num = (a * a + b * b) - r * r; //计算画的点离圆心的距离
		if (num > 0)
		{
			b--;
			a--;
		}
	}
}

/******************************************************
 * 函数功能: 设置一个8x8单元格的内容
 *   逻辑上, 将oled屏幕划分为逻辑行和逻辑列.
 *   - 逻辑行: 每8个纵向像素划分为1行, 包括128个横
 * 向像素, 逻辑行实际上与SSD1306的页相同.
 *   - 逻辑列: 每8个横向像素组成一个逻辑列, 跨越8行. 实际
 * 上, 逻辑列就是1个ascii码字符占有的宽度
 *   - 逻辑单元格: 即1个8x8像素
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * @param - col, 逻辑列号, 取值范围 0~15
 * @param - data, 要写入的内容缓冲区
 * @param - len, 要写入的内容长度, 取值范围 0~7, 8及以上
 *               的部分忽略
 * ------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Set_8x8_Cell(u8 row, u8 col, u8 *data, u8 len, u8 mode)
{
    int i = 0;
    u8 x = (col % ASCII_CHAR_COUNT_PER_LINE) * ASCII_WIDTH;
    row = row % PAGE_COUNT;
    len = len % (ASCII_WIDTH + 1);

    memcpy(&s_OLED_GRAM[row][x], data, len);

    if (mode & OLED_ROW_HIGH_LIGHT_ON)
    {
        for (i = 0; i < len; i++)
        {
            s_OLED_GRAM[row][x + i] = ~s_OLED_GRAM[row][x + i];
        }
    }

    if (mode & OLED_UP_LINE)
    {
        for (i = 0; i < len; i++)
        {
            s_OLED_GRAM[row][x + i] = s_OLED_GRAM[row][x + i] | 0x01;
        }
    }

    if (mode & OLED_BOTTOM_LINE)
    {
        for (i = 0; i < len; i++)
        {
            s_OLED_GRAM[row][x + i] = s_OLED_GRAM[row][x + i] | 0x80;
        }
    }

    if (mode & OLED_LEFT_LINE)
    {
        s_OLED_GRAM[row][x] = 0xFF;
    }

    if (mode & OLED_RIGHT_LINE)
    {
        s_OLED_GRAM[row][x + len - 1] = 0xFF;
    }
}

/******************************************************
 * 函数功能: 清除一个8x8单元格的内容
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * @param - col, 逻辑列号, 取值范围 0~15
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Clear_8x8_Cell(int fd, u8 row, u8 col)
{
    u8 d[ASCII_WIDTH] = { 0 };
    OLED_Set_8x8_Cell(row, col, d, ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);
}

/******************************************************
 * 函数功能: 清除一行的内容
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Clear_Row(u8 row)
{
    row = row % PAGE_COUNT;
    memset(&s_OLED_GRAM[row][0], 0, sizeof(s_OLED_GRAM[row % PAGE_COUNT]));
}

/******************************************************
 * 函数功能: 点亮一行
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Light_Row(u8 row)
{
    u8 i;
    u8 d[ASCII_WIDTH];

    memset(d, 0xFF, ASCII_WIDTH);

    for (i = 0; i < ASCII_CHAR_COUNT_PER_LINE; i++)
    {
        OLED_Set_8x8_Cell(row, i, d, ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_ON);
    }
}

/******************************************************
 * 函数功能: 清屏
 * ---------------------------------------------------
 * @param - 无
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Clear()
{
	memset(s_OLED_GRAM, 0, sizeof(s_OLED_GRAM));
}

/******************************************************
 * 函数功能: 点亮整个屏
 * ---------------------------------------------------
 * @param - 无
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_LIGHT_ALL()
{
	memset(&s_OLED_GRAM[0][0], 0xFF, sizeof(s_OLED_GRAM));
}

/******************************************
 * 函数功能: 显示一个8x16的点阵ASCII码字符
 * ----------------------------------------
 * @param - row, 逻辑行号
 * @param - col, 逻辑列号
 * ----------------------------------------
 * @return - 无
 ******************************************/
void OLED_Show8x16Char(u8 row, u8 col, u8 c, u8 mode)
{
    c = c - ' '; //得到字库中的索引

    switch (mode)
    {
        case OLED_UP_LINE:
            OLED_Set_8x8_Cell(row, col, (u8*) (F8X16 + c * 16), ASCII_WIDTH, OLED_UP_LINE);                                 //显示上半部分
            OLED_Set_8x8_Cell(row + 1, col, (u8*) (F8X16 + c * 16 + PAGE_HEIGHT), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);    //显示下半部分
            break;
        case OLED_BOTTOM_LINE:
            OLED_Set_8x8_Cell(row, col, (u8*) (F8X16 + c * 16), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);                                 //显示上半部分
            OLED_Set_8x8_Cell(row + 1, col, (u8*) (F8X16 + c * 16 + PAGE_HEIGHT), ASCII_WIDTH, OLED_BOTTOM_LINE);    //显示下半部分
            break;
        case OLED_RECTANGEL_LINE:
            OLED_Set_8x8_Cell(row, col, (u8*) (F8X16 + c * 16), ASCII_WIDTH, OLED_UP_LEFT_RIGHT_LINE);                                 //显示上半部分
            OLED_Set_8x8_Cell(row + 1, col, (u8*) (F8X16 + c * 16 + PAGE_HEIGHT), ASCII_WIDTH, OLED_BOTTOM_LEFT_RIGHT_LINE);
            break;
        case OLED_ROW_HIGH_LIGHT_ON:
        case OLED_ROW_HIGH_LIGHT_OFF:
        case OLED_RIGHT_LINE:
        case OLED_LEFT_LINE:
        default:
            OLED_Set_8x8_Cell(row, col, (u8*) (F8X16 + c * 16), ASCII_WIDTH, mode);                     //显示上半部分
            OLED_Set_8x8_Cell(row + 1, col, (u8*) (F8X16 + c * 16 + PAGE_HEIGHT), ASCII_WIDTH, mode);   //显示下半部分
            break;
    }
}

/******************************************
 * 函数功能: 显示一个16x16的点阵汉字字符
 * !!本函数通过 Unicode 编码来查找字模
 * ----------------------------------------
 * @param - row, 逻辑行号
 * @param - col, 逻辑列号
 * ----------------------------------------
 * @return - 无
 ******************************************/
void OLED_Show16x16Chinese_UTF8(u8 row, u8 col, u32 c, u8 mode)
{
    char buf[32] = { 0 };
    int len = getFontUTF8(c, buf);

    if (len > 0)
    {
        switch (mode)
        {
            case OLED_UP_LINE:
                OLED_Set_8x8_Cell(row, col, (u8*) (buf), ASCII_WIDTH, OLED_UP_LINE);                           //显示左上部分
                OLED_Set_8x8_Cell(row, col + 1, (u8*) (buf + ASCII_WIDTH), ASCII_WIDTH, OLED_UP_LINE);         //显示右上部分
                OLED_Set_8x8_Cell(row + 1, col, (u8*) (buf + 2 * ASCII_WIDTH), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);     //显示左下部分
                OLED_Set_8x8_Cell(row + 1, col + 1, (u8*) (buf + 3 * ASCII_WIDTH), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF); //显示右下部分
                break;
            case OLED_BOTTOM_LINE:
                OLED_Set_8x8_Cell(row, col, (u8*) (buf), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);                    //显示左上部分
                OLED_Set_8x8_Cell(row, col + 1, (u8*) (buf + ASCII_WIDTH), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);  //显示右上部分
                OLED_Set_8x8_Cell(row + 1, col, (u8*) (buf + 2 * ASCII_WIDTH), ASCII_WIDTH, OLED_BOTTOM_LINE);     //显示左下部分
                OLED_Set_8x8_Cell(row + 1, col + 1, (u8*) (buf + 3 * ASCII_WIDTH), ASCII_WIDTH, OLED_BOTTOM_LINE); //显示右下部分
                break;
            case OLED_LEFT_LINE:
                OLED_Set_8x8_Cell(row, col, (u8*) (buf), ASCII_WIDTH, OLED_LEFT_LINE);                    //显示左上部分
                OLED_Set_8x8_Cell(row, col + 1, (u8*) (buf + ASCII_WIDTH), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);  //显示右上部分
                OLED_Set_8x8_Cell(row + 1, col, (u8*) (buf + 2 * ASCII_WIDTH), ASCII_WIDTH, OLED_LEFT_LINE);     //显示左下部分
                OLED_Set_8x8_Cell(row + 1, col + 1, (u8*) (buf + 3 * ASCII_WIDTH), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF); //显示右下部分
                break;
            case OLED_RIGHT_LINE:
                OLED_Set_8x8_Cell(row, col, (u8*) (buf), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);                    //显示左上部分
                OLED_Set_8x8_Cell(row, col + 1, (u8*) (buf + ASCII_WIDTH), ASCII_WIDTH, OLED_RIGHT_LINE);  //显示右上部分
                OLED_Set_8x8_Cell(row + 1, col, (u8*) (buf + 2 * ASCII_WIDTH), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_OFF);     //显示左下部分
                OLED_Set_8x8_Cell(row + 1, col + 1, (u8*) (buf + 3 * ASCII_WIDTH), ASCII_WIDTH, OLED_RIGHT_LINE); //显示右下部分
                break;
            case OLED_RECTANGEL_LINE:
                OLED_Set_8x8_Cell(row, col, (u8*) (buf), ASCII_WIDTH, OLED_UP_LEFT_LINE);                               //显示左上部分
                OLED_Set_8x8_Cell(row, col + 1, (u8*) (buf + ASCII_WIDTH), ASCII_WIDTH, OLED_UP_RIGHT_LINE);            //显示右上部分
                OLED_Set_8x8_Cell(row + 1, col, (u8*) (buf + 2 * ASCII_WIDTH), ASCII_WIDTH, OLED_BOTTOM_LEFT_LINE);     //显示左下部分
                OLED_Set_8x8_Cell(row + 1, col + 1, (u8*) (buf + 3 * ASCII_WIDTH), ASCII_WIDTH, OLED_BOTTOM_RIGHT_LINE);//显示右下部分
                break;
            case OLED_ROW_HIGH_LIGHT_ON:
            case OLED_ROW_HIGH_LIGHT_OFF:
            default:
                OLED_Set_8x8_Cell(row, col, (u8*) (buf), ASCII_WIDTH, mode);                           //显示左上部分
                OLED_Set_8x8_Cell(row, col + 1, (u8*) (buf + ASCII_WIDTH), ASCII_WIDTH, mode);         //显示右上部分
                OLED_Set_8x8_Cell(row + 1, col, (u8*) (buf + 2 * ASCII_WIDTH), ASCII_WIDTH, mode);     //显示左下部分
                OLED_Set_8x8_Cell(row + 1, col + 1, (u8*) (buf + 3 * ASCII_WIDTH), ASCII_WIDTH, mode); //显示右下部分
                break;
        }
    }
    else
    {
        OLED_Show8x16Char(row, col, '?', mode);
        OLED_Show8x16Char(row, col + 1, '?', mode);
    }
}

/******************************************************
 * 函数功能: 显示显示BMP图片
 * ---------------------------------------------------
 * @param - x0, 起始像素的横坐标, 0~127
 * @param - y0, 起始页的范围0～7
 * @param - x1: 结束像素的横坐标, 0~127
 * @param - y1, 结束页的范围0～7
 * @param - BMP, 位图数组
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
/***********功能描述：128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7*****************/
void OLED_DrawBMP(int fd, u8 x0, u8 y0, u8 x1, u8 y1, u8 BMP[], u8 mode)
{
	unsigned int j = 0;
	u8 x, y;

	if (y1 % 8 == 0)
	{
		y = y1 / 8;
	}
	else
	{
		y = y1 / 8 + 1;
	}

	for (y = y0; y < y1; y++)
	{
		for (x = x0; x < x1; x++)
		{
			s_PIXEL_GRAM[x][y] = BMP[j++];
		}
	}
}

/******************************************************
 * 函数功能: 打印一个 UTF-8 字符串.
 * 与 OLED_Print() 功能类似, 唯一不同的是, 字符串 s 是
 * 以 UTF-8 编码的.
 * 如果本文件(或者其他调用液晶打印的文件)是以 UTF-8 编
 * 码的, 如果想往液晶输出带汉字的打印信息, 应调用此函数
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * @param - col, 逻辑列号, 取值范围 0~15
 * @param - width, 数据项描述宽度, 以逻辑列为单位, 取值
 *          范围1~15, 即至少1列, 至多15列, 留1列给数值.
 * @param - s, 要显示的字符串
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Print_UTF8(u8 row, u8 col, u8 width, char *s, u8 mode)
{
    u32 i;           //字符串索引
    u8 delta;        //每次的列增量
    u32 unicode = 0; // Unicode 编码值
    u8 bytes = 0;    // utf-8字节数
    u32 length = 0;  //取字符串总长
    char *p = s;     //字符串

    u8 curRow = row;
    u8 curCol = col;

    if (NULL == s)
    {
        return;
    }

    length = strlen(s);

    for (i = 0; i < length; i += bytes, curCol += delta)
    {
        bytes = utf8_to_unicode(p + i, length - i, &unicode);

        if (1 == bytes) // 1个字节编码, 是ASCII符号
        {
            delta = 1;
        }
        else if (bytes >= 2) //大于等于2个字节, 为汉字
        {
            delta = 2;
        }

        if (((curCol - col) + delta) > width)
        {
            if ((curRow + CHAR_LOGICAL_HEIGHT) >= LOGICAL_ROW_COUNT)
            {
                curRow = 0;
            }
            else
            {
                curRow += CHAR_LOGICAL_HEIGHT;
            }

            curCol = col;
        }

        if (1 == delta) // ASCII码
        {
            if (p[i] == '\n')
            {
                curRow += CHAR_LOGICAL_HEIGHT;
                curCol = 0;
                delta = 0;
            }
            else
            {
                OLED_Show8x16Char(curRow, curCol, p[i], mode);
            }
        }
        else if (2 == delta) //汉字
        {
            OLED_Show16x16Chinese_UTF8(curRow, curCol, unicode, mode);
        }
    }

    s_oled_layout.curRow = curRow;
    s_oled_layout.curCol = curCol;
}

/******************************************************
 * 函数功能: 获取一个字符串中的一个字符在屏幕上的位置.
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7, 此字符串的第1个字符的
 *               所在行号
 * @param - col, 逻辑列号, 取值范围 0~15, 此字符串的第1个字符的
 *               所在列号
 * @param - pPosition, 字符串位置信息, 输入时必须指定成员内的
 *          pString为所查询的字符串, charIdx为所查询的字符索引.
 *          函数如果正确返回, 则在其成员的curRow和curCol中存储
 *          要查找的字符的位置信息, byteIdx存储要查找的字符编码
 *          在字符串中的字节索引值.
 * @param - pUnicode, 如果成功查找到, 则存储该字符的Unicode编码
 * ---------------------------------------------------
 * @return - 查找成功 - 返回字符的UTF-8编码长度;
 *           查找失败 - 返回-1
 ******************************************************/
int OLED_getPositionByIndex(u8 row, u8 col, u8 width, oledStringPosition_s *pPosition, u32 *pUnicode)
{
    if (pPosition == NULL || pPosition->pString == NULL || pPosition->charIdx < 0 || pUnicode == NULL)
    {
        return -1;
    }

    u32 i = 0;       //字符串索引
    u8 delta = 0;    //每次的列增量
    u8 bytes = 0;    //utf-8字节数
    u32 length = 0;  //取字符串总长
    char *p = pPosition->pString;     //字符串
    int curIdx = 0;
    u8 curRow = row;
    u8 curCol = col;

    length = strlen(pPosition->pString);

    for (i = 0, curIdx = 0; i < length; i += bytes, curCol += delta, curIdx++)
    {
        bytes = utf8_to_unicode(p + i, length - i, pUnicode);

        if (1 == bytes) // 1个字节编码, 是ASCII符号
        {
            delta = 1;
        }
        else if (bytes >= 2) //大于等于2个字节, 为汉字
        {
            delta = 2;
        }

        if (curIdx == pPosition->charIdx)
        {
            pPosition->byteIdx = i;
            pPosition->curRow = curRow;
            pPosition->curCol = curCol;
            return delta;
        }

        if (((curCol - col) + delta) > width)
        {
            if ((curRow + CHAR_LOGICAL_HEIGHT) >= LOGICAL_ROW_COUNT)
            {
                curRow = 0;
            }
            else
            {
                curRow += CHAR_LOGICAL_HEIGHT;
            }

            curCol = col;
        }

        if (p[i] == '\n')
        {
            curRow += CHAR_LOGICAL_HEIGHT;
            curCol = 0;
            delta = 0;
        }
    }

    return -1;
}

/******************************************************
 * 函数功能: 设置一个字符串中的一个字符为高亮或者普通
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7, 此字符串的第1个字符的
 *               所在行号
 * @param - col, 逻辑列号, 取值范围 0~15, 此字符串的第1个字符的
 *               所在列号
 * @param - s, 字符串字符
 * @param - idx, 要高亮的字符, 在s中的索引值
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_LightOrNormalCharByIndex(u8 row, u8 col, u8 width, char *s, int idx, u8 mode)
{
    if (NULL == s || idx < 0)
    {
        return;
    }

    oledStringPosition_s pos = { .pString = s, .charIdx = idx };
    u32 unicode = 0;

    int res = OLED_getPositionByIndex(row, col, width, &pos, &unicode);

    if (1 == res) // ASCII码
    {
        OLED_Show8x16Char(pos.curRow, pos.curCol, unicode, mode);
    }
    else if (2 == res) //汉字
    {
        OLED_Show16x16Chinese_UTF8(pos.curRow, pos.curCol, unicode, mode);
    }
}

/******************************************************
 * 函数功能: 获取给定数d模mod的余数. mod必须大于1, 否则返回-1.
 * 根据余数的定义, d = n * mod + r, 0 <= r < mod, r为余数,
 * 则返回的余数必须介于 0 ~ mod-1 之间,
 * ---------------------------------------------------
 * @param - d, 被求余数的值
 * @param - mod, 模的值
 * ---------------------------------------------------
 * @return - 余数r, 且 0 <= r < mod
 ******************************************************/
int OLED_getReminder(int d, int mod)
{
    if (mod <= 1)
    {
        return -1;
    }

    if (d >= 0)
    {
        return d % mod;
    }

    int abs = d * -1;
    return mod - abs % mod;
}

/******************************************************
 * 函数功能: 设置一个字符串中的一个字符增长或减少
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7, 此字符串的第1个字符的
 *               所在行号
 * @param - col, 逻辑列号, 取值范围 0~15, 此字符串的第1个字符的
 *               所在列号
 * @param - s, 字符串字符
 * @param - idx, 要高亮的字符, 在s中的索引值
 * @param - incre, 要增加的值
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_increaseCharByIndex(u8 row, u8 col, u8 width, char *s, int idx, int incre, int base, u8 mode)
{
    if (NULL == s || idx < 0)
    {
        return;
    }

    oledStringPosition_s pos = { .pString = s, .charIdx = idx };
    u32 unicode = 0;

    int res = OLED_getPositionByIndex(row, col, width, &pos, &unicode);

    if (1 == res) // ASCII码
    {
        //16进制数, 或者10进制数.
        //且当前字符在 '0' ~ '9' || 'a'~'f' || 'A'~'F' 之间
        if (
                (
                     base == 10 ||
                     base == 16
                ) &&
                (
                    (unicode >= '0' && unicode <= '9') ||
                    (unicode >= 'a' && unicode <= 'f') ||
                    (unicode >= 'A' && unicode <= 'F')
                )
           )
        {
            incre = OLED_getReminder(incre, base);
            int idx = 0;
            if (unicode >= '0' && unicode <= '9')
            {
                idx = unicode - '0';
            }
            else if (unicode >= 'a' && unicode <= 'f')
            {
                idx = unicode - 'a' + 10;
            }
            else if (unicode >= 'A' && unicode <= 'F')
            {
                idx = unicode - 'A' + 10;
            }

            idx = ((idx + incre) % base);
            unicode = s_OLED_upperHexLetter[idx];
            OLED_Show8x16Char(pos.curRow, pos.curCol, unicode, mode);
            s[pos.byteIdx] = unicode;
        }
    }
}

/******************************************************
 * 函数功能: 打印一个 UTF-8 字符串.可以在指定位置进行高亮显示
 * 如果本文件(或者其他调用液晶打印的文件)是以 UTF-8 编
 * 码的, 如果想往液晶输出带汉字的打印信息, 应调用此函数
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * @param - col, 逻辑列号, 取值范围 0~15
 * @param - width, 数据项描述宽度, 以逻辑列为单位, 取值
 *          范围1~15, 即至少1列, 至多15列, 留1列给数值.
 * @param - s, 要显示的字符串
 * @param    mode: OLED_ROW_HIGH_LIGHT_ON  OLED_ROW_HIGH_LIGHT_OFF
 *  @param  start 哪一列开始高亮  mode=OLED_ROW_HIGH_LIGHT_ON 有效
 *  @param  end 哪一列结束高亮显示  ，高亮显示范围为 start----end
 *
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Print_UTF8_StarttoEndHighLight(u8 row, u8 col, u8 width, char *s,   u8 start, u8 end)
{
    u32 i;           //字符串索引
    u8 delta;        //每次的列增量
    u32 unicode = 0; // Unicode 编码值
    u8 bytes = 0;    // utf-8字节数
    u32 length = 0;  //取字符串总长
    char *p = s;     //字符串

    u8 curRow = row;
    u8 curCol = col;

    if (NULL == s)
    {
        return;
    }

    length = strlen(s);

    for (i = 0; i < length; i += bytes, curCol += delta)
    {
        bytes = utf8_to_unicode(p + i, length - i, &unicode);

        if (1 == bytes) // 1个字节编码, 是ASCII符号
        {
            delta = 1;
        }
        else if (bytes >= 2) //大于等于2个字节, 为汉字
        {
            delta = 2;
        }

        if (((curCol - col) + delta) > width)
        {
            if ((curRow + CHAR_LOGICAL_HEIGHT) >= LOGICAL_ROW_COUNT)
            {
                curRow = 0;
            }
            else
            {
                curRow += CHAR_LOGICAL_HEIGHT;
            }

            curCol = col;
        }

        if (1 == delta) // ASCII码
        {
            if (p[i] == '\n')
            {
                curRow += CHAR_LOGICAL_HEIGHT;
                curCol = 0;
                delta = 0;
            }
            else
            {
                if(curCol>=start && curCol<=end )
                {
                       p[i] = p[i] - ' '; //得到字库中的索引
                       OLED_Set_8x8_Cell(row, col, (u8*) (F8X16 + p[i] * 16), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_ON);                   //显示上半部分
                       OLED_Set_8x8_Cell(row + 1, col, (u8*) (F8X16 + p[i] * 16 + PAGE_HEIGHT), ASCII_WIDTH, OLED_ROW_HIGH_LIGHT_ON); //显示下半部分

                }else
                {
                    OLED_Show8x16Char(curRow, curCol, p[i], OLED_ROW_HIGH_LIGHT_OFF);
                }

            }
        }
        else if (2 == delta) //汉字 不支持 高亮
        {
            //OLED_Show16x16Chinese_UTF8(curRow, curCol, unicode, mode);
        }
    }

    s_oled_layout.curRow = curRow;
    s_oled_layout.curCol = curCol;
}

/******************************************************
 * 函数功能: 计算UTF-8字符串所占用的逻辑行数
 * ---------------------------------------------------
 * @param - s, 字符串指针
 * @param - width, 一行的宽度
 * @param - padRow, 是否要增加一个空行, 取值范围{0,1}
 * ---------------------------------------------------
 * @return - 字符串占用的行数
 ******************************************************/
u16 calc_row_count_utf8(char *s, u8 width, u8 padRow)
{
	u32 i = 0;       //索引
	u8 col = 0;      //列
	u16 row = 0;     //行数
	int delta = 0;   //列增量
	u8 bytes = 0;    //一个utf-8编码的字节数
	u32 unicode = 0; //用于调用utf8_to_unicode(), 没有用处

	if (NULL == s)
	{
		return 0;
	}

	char *p = s;
	u32 length = strlen(p); //取字符串总长

	//如果字符串长度大于0, 则至少要占2个逻辑行
	if (length > 0)
	{
		row += CHAR_LOGICAL_HEIGHT;
	}

	for (i = 0; i < length; i += bytes, col += delta)
	{
		bytes = utf8_to_unicode(p + i, length - i, &unicode);

		if (1 == bytes) // 1个字节编码, 是ASCII符号
		{
			delta = 1;
		}
		else if (bytes >= 2) //大于等于2个字节, 为汉字
		{
			delta = 2;
		}

		if ((col + delta) > width)
		{
			row += CHAR_LOGICAL_HEIGHT;
			col = 0;
		}
	}

	//只有row大于0, 才需要增加1个行数
	if (row > 0)
	{
		row += padRow;
	}

	return row;
}

/******************************************************
 * 函数功能: 计算 UTF-8 编码所用的字节数
 * 0xxxxxxx 返回0
 * 10xxxxxx 不存在, 返回-1
 * 110xxxxx 返回2
 * 1110xxxx 返回3
 * 11110xxx 返回4
 * 111110xx 返回5
 * 1111110x 返回6
 * ---------------------------------------------------
 * @param - c, UTF-8 编码的第一个字节
 * ---------------------------------------------------
 * @return - 成功, 返回 UTF-8 编码所用的字节数
 *           失败, 返回0
 ******************************************************/
s8 get_utf8_bytes(u8 c)
{
	if ((c & 0x80) == 0)
	{
		return 0;
	}

	if (c >= 0xC0 && c < 0xE0)
	{
		return 2;
	}

	if (c >= 0xE0 && c < 0xF0)
	{
		return 3;
	}

	if (c >= 0xF0 && c < 0xF8)
	{
		return 4;
	}

	if (c >= 0xF8 && c < 0xFC)
	{
		return 5;
	}

	if (c >= 0xFC)
	{
		return 6;
	}

	return -1;
}

/******************************************************
 * 函数功能: 将 UTF-8 编码的字符转换为 Unicode 索引值
 * !!本程序只适用于小端平台!!
 * ---------------------------------------------------
 * @param[in]  - s, UTF-8 编码的字符串, 英文字符或中文字符
 * @param[in]  - len, 字符串长度
 * @param[out] - unicode, 成功, 返回Unicode 索引值;
 *                        失败, 返回0
 * ---------------------------------------------------
 * @return - 成功, 返回本次转换消耗的字节数量
 *           失败, 返回0
 ******************************************************/
u8 utf8_to_unicode(char *s, u16 len, u32 *unicode)
{
	if (NULL == s || NULL == unicode || 0 == len)
	{
		return 0;
	}

	char *pInput = s;
	u8 b1 = 0;
	u8 b2 = 0;
	u8 b3 = 0;
	u8 b4 = 0;
	u8 b5 = 0;
	u8 b6 = 0;
	*unicode = 0;
	u8 *pOutput = (u8*) unicode;
	s8 i = get_utf8_bytes(pInput[0]);

	if (-1 == i)
	{
		return 0;
	}

	if (0 == i)
	{
		*pOutput = pInput[0];
		i = 1;
	}
	else if (i == 2)
	{
		b1 = *pInput;
		b2 = *(pInput + 1);

		if ((b2 & 0xE0) != 0x80)
		{
			return 0;
		}

		/*******************************
		 * 从UTF-8的最后1个字节开始处理,
		 * b1 左移6位, 目的是将b1的低2位
		 * 移至Unicode低字节的最高2位,
		 * 再与b2的后6位相加即可, 下同.
		 * 因为面向小端平台, 故处理顺序:
		 * 低字节
		 * .
		 * .
		 * .
		 * 高字节
		 ***********/
		*pOutput = (b1 << 6) + (b2 & 0x3F); // b1保留低2位, 作为低字节的高2位, b2保留低6位
		*(pOutput + 1) = (b1 >> 2) & 0x07;  //取b1剩余的3位, 存入高字节
	}
	else if (i == 3)
	{
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);

		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
		{
			return 0;
		}

		*pOutput = (b2 << 6) + (b3 & 0x3F);       // b2保留低2位, 作为低字节的高2位, b3保留低6位
		*(pOutput + 1) = (b1 << 4) + ((b2 >> 2) & 0x0F); //取b1的4位, 与b2的除了0b110以外高4位组合为高字节
	}
	else if (i == 4)
	{
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);

		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
				|| ((b4 & 0xC0) != 0x80))
		{
			return 0;
		}

		*pOutput = (b3 << 6) + (b4 & 0x3F);
		*(pOutput + 1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
		*(pOutput + 2) = ((b1 << 2) & 0x1C) + ((b2 >> 4) & 0x03);
	}
	else if (i == 5)
	{
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		b5 = *(pInput + 4);

		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
				|| ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80))
		{
			return 0;
		}

		*pOutput = (b4 << 6) + (b5 & 0x3F);
		*(pOutput + 1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
		*(pOutput + 2) = (b2 << 2) + ((b3 >> 4) & 0x03);
		*(pOutput + 3) = (b1 << 6);
	}
	else if (i == 6)
	{
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		b5 = *(pInput + 4);
		b6 = *(pInput + 5);

		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
				|| ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)
				|| ((b6 & 0xC0) != 0x80))
		{
			return 0;
		}

		*pOutput = (b5 << 6) + (b6 & 0x3F);
		*(pOutput + 1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
		*(pOutput + 2) = (b3 << 2) + ((b4 >> 4) & 0x03);
		*(pOutput + 3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
	}

	return (u8) i;
}

void getInput(void)
{
    int c;
    fcntl(0, F_SETFL, O_NONBLOCK);
    while (1)
    {
        c = fgetc(stdin);
        if(c > 0)
        {
            printf("The number you typed was %d\n", c);
            sleep(3);
        }
        else
        {
            printf("nothing input\n");
        }

        sleep(1);
    }
}

void OLED_test(void)
{
    openFontDB("/home/floyd/repo/mytesting/db/font.db");
    OLED_Print_UTF8(0, 0, 16, "大開眼界", OLED_BOTTOM_LINE);
    OLED_Print_UTF8(2, 0, 16, "abcde", OLED_BOTTOM_LINE);
    OLED_Print_UTF8(4, 0, 16, "abcde", OLED_BOTTOM_LINE);
    closeFontDB();

//    OLED_DrawLine(0, 0, 10, 8, 1);
//    OLED_DrawCircle(9,9,5);
    OLED_Refresh();
}
