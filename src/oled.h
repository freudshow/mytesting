/******************************************************************************
 作 者 : 宋宝善
 版本号 : 1.0
 生成日期: 2021年3月1日
 概	  述:
 1. oled液晶主程序

 修改日志:
 1.日期: 2021年3月1日
 修改内容: 创建文件
 ******************************************************************************/


#ifndef __OLED_H
#define __OLED_H

#include "basedef.h"

#define OLED_IIC_DEV    "/dev/i2c-1"

#define OLED_ABS(x,y)   ((x) >= (y)) ? ((x) - (y)) : ((y) - (x))

#define PAGE_HEIGHT 8                   //SSD1306中的一页的高度
#define XLevelL		0x00
#define XLevelH		0x10
#define MAX_COLUMN	128                 //屏幕的横向像素数
#define MAX_ROW		64                  //屏幕的纵向像素数
#define	BRIGHTNESS	0xFF

#define PAGE_COUNT                      (MAX_ROW/PAGE_HEIGHT)               //屏幕的页数, 或者行数
#define ASCII_WIDTH                     8                                   //一个ascii码字符占用的像素个数
#define ASCII_CHAR_COUNT_PER_LINE       (MAX_COLUMN/ASCII_WIDTH)            //一行显示的ascii码字符个数, 16
#define CHINESE_CHAR_WIDTH              2                                   //一个汉字占2个ascii码字符宽度
#define CHAR_HEIGHT                     (2*PAGE_HEIGHT)                     //不论ascii码字符还是汉字字符, 都使用高度为16的字库
#define CHAR_LOGICAL_HEIGHT             2                                   //一个字符占有的逻辑行数
#define CHINESE_CHAR_COUNT_PER_LINE     (ASCII_CHAR_COUNT_PER_LINE/CHINESE_CHAR_WIDTH)   //一行显示的汉字字符个数, 8

#define LOGICAL_ROW_COUNT               PAGE_COUNT                          //逻辑行总数, 8
#define LOGICAL_COL_COUNT               ASCII_CHAR_COUNT_PER_LINE           //逻辑列总数, 16

#define UNIT_CHAR_COUNT                 2                                   //单位的ASCII码个数

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

typedef struct oled_key_queen {
#define OLED_KEY_QUEEN_LEN  16  //按键队列长度
#define OLED_KEY_INVALID    0   //无效键值
#define OLED_KEY_UP         1   //上键
#define OLED_KEY_DOWN       2   //下键
#define OLED_KEY_LEFT       3   //左键
#define OLED_KEY_RIGHT      4   //右键
#define OLED_KEY_ENTER      5   //确定键
        u8 keys[OLED_KEY_QUEEN_LEN]; //按键操作队列
        u8 capacity;                //队列的容量
        u8 head;                    //写入指针
        u8 tail;                    //读出指针
        u8 len;                     //队列有效长度, 当有效长度为0时, 表示按键操作已处理完; 当有效长度满时, 不再写入新的按键操作
#define NEED_HANDLE_KEY     1
#define NOT_NEED_HANDLE_KEY 0
        u8 needHandle;              //1-有按键事件需要处理, 0-没有按键事件需要处理
} oled_key_queen_s;


typedef struct oled_show_item {
        u16 pageNo;                     //显示页码
        u8 rowNo;                      //数据项描述开始行号
        u16 rowCount;                   //数据项描述占用几行
        u8 valueRowNo;                 //数值开始行号, 数值只允许占用1行

        u32 dbSeq;                     //实时库序号
        double dead;                    //死区值, 前后变化超过这个数才刷新显示, 否则不刷新显示
        double ratio;                   //比例, 拿到实时库数据后, 乘以这个比例, 再显示出来
        char unit[UNIT_CHAR_COUNT + 1];   //数据项的单位, 留一个结束符'\0'
        char desc[64];                  //数据项名称

        float value;        //要显示的数值
} oled_show_item_s;

typedef struct oled_show_list {
        oled_show_item_s *pList;  //要显示的数据列表
        u32 listCnt;     //要显示的数据的个数

        u32 period;      //周期显示时, 每隔多久循环一次, 单位100毫秒(ms)的整数倍
        u8 descWidth;   //数据描述的宽度
#define NEED_SPLIT_ROW      1
#define NOT_NEED_SPLIT_ROW  0
        u8 splitRow;    //两个数据项之间是否需要空1行. 1-需要空1行; 0-不需要空1行
#define SUPORT_KEY_PAGE_ROLL      1
#define NOT_SUPORT_KEY_PAGE_ROLL  0
        u8 supHardKey;  //1-支持按键翻页, 0-不支持
        u8 pageKeep;    //页面保持时间, 单位毫秒
        u32 pageKeepCount;    //页面保持时间要经过多少个循环周期
        u32 loops;    //进入周期轮询的次数

        u32 pageCnt;     //总共显示多少页
        u32 curPage;     //当前显示页号, 0-based
        u32 prePage;     //前一次显示页号, 0-based
        u32 curRow;      //当前未被占据的行号, 逻辑行号, 0-based
        u32 totalRows;   //总共需要显示的行数
} oled_show_list_s;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

//OLED控制用函数
    extern void OLED_Display_On();
    extern void OLED_Display_Off();
    extern void OLED_Set_8x8_Cell(u8 row, u8 col, u8 *data, u8 len);
    extern void OLED_Clear();
    void OLED_LIGHT_ALL();
    void OLED_Light_Row(u8 row);
    void OLED_Clear_Row(u8 row);
    void OLED_ColorTurn();
    void OLED_Refresh();
    void OLED_DrawPoint(u8 x, u8 y, u8 t);

    extern void OLED_Print(u8 row, u8 col, u8 width, char *s);
    extern void OLED_Print_UTF8(u8 row, u8 col, u8 width, char *s);
    extern void OLED_Init(int fd);

    extern int parse_oled_config(unsigned char linkno, const char *cxxxxjson);
    extern u16 calc_row_count_utf8(char *s, u8 width, u8 padRow);
    extern u8 utf8_to_unicode(char *s, u16 len, u32 *unicode);

    extern int push_oled_key_queen(u8 key);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif
