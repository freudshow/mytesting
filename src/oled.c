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
#include "oledfont.h"
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

static u8 s_OLED_GRAM[PAGE_COUNT][MAX_COLUMN] = { 0 };
static u8 s_PIXEL_GRAM[MAX_ROW][MAX_COLUMN] = { 0 };
static int s_oled_fd;

/******************************************************
 * 函数功能: 向 IIC 总线写入一个字节的数据
 * ---------------------------------------------------
 * @param - IIC_Data, 要写入的数据
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
int i2c_write_data(__u16 slave_addr, __u8 reg_addr, __u8 reg_data)
{
	int ret;
	u8 buf[2];
	struct i2c_rdwr_ioctl_data i2c_write_rtc;   //定义I2C数据包结构体
	buf[0] = reg_addr;
	buf[1] = reg_data;
	struct i2c_msg msg[1] = {                   //构建i2c_msg结构体
			[0] = { .addr = slave_addr, .flags = 0, .buf = buf, .len =
					sizeof(buf) }, };
	i2c_write_rtc.msgs = msg;                   //要发送的数据包的指针
	i2c_write_rtc.nmsgs = 1;                        //要发送的数据包的个数
	ret = ioctl(s_oled_fd, I2C_RDWR, &i2c_write_rtc);  //使用ioctl的I2C_RDWR进行数据传输
	if (ret < 0)
	{
		perror("ioctl error is:");
		return ret;
	}
	return ret;
}

int i2c_read_data(__u16 slave_addr, __u8 reg_addr, __u8 *rx_data, __u16 rxlen)
{
	int ret;
	struct i2c_rdwr_ioctl_data i2c_read_rtc;    //定义I2C数据包结构体

	struct i2c_msg msg[2] = {                   //构建i2c_msg结构体
			[0] = { .addr = slave_addr, .flags = 0, .buf = &reg_addr, .len =
					sizeof(reg_addr) }, [1] = { .addr = slave_addr, .flags = 1,
					.buf = rx_data, .len = rxlen } };

	i2c_read_rtc.msgs = msg;                    //要发送的数据包的指针
	i2c_read_rtc.nmsgs = 2;                     //要发送的数据包的个数

	ret = ioctl(s_oled_fd, I2C_RDWR, &i2c_read_rtc);   //使用ioctl的I2C_RDWR进行数据传输
	if (ret < 0)
	{
		perror("ioctl error is:");
		return ret;
	}
	return ret;
}

/******************************************************
 * 函数功能: 向 IIC 总线写入一个字节的命令
 * ---------------------------------------------------
 * @param - IIC_Command, 要写入的命令
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void Write_IIC_Command(u8 IIC_Command)
{
	i2c_write_data(0x3c, 0x0, IIC_Command);
}

/******************************************************
 * 函数功能: 向 IIC 总线写入一个字节的数据
 * ---------------------------------------------------
 * @param - IIC_Data, 要写入的数据
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void Write_IIC_Data(u8 IIC_Data)
{
	i2c_write_data(0x3c, 0x40, IIC_Data);
}

/******************************************************
 * 函数功能: 向 IIC 总线写入一个字节的数据/命令
 * ---------------------------------------------------
 * @param - dat, 要写入的数据
 * @param - cmd, 要写入的是否为数据.
 *               1-要写入的是数据; 其他值-要写入的是命令
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
	if (cmd)
	{
		Write_IIC_Data(dat);
	}
	else
	{
		Write_IIC_Command(dat);
	}
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

/**********************************************
 * 初始化SSD1306
 * -------------------------------------------
 * @param - 无
 * -------------------------------------------
 * @return - 无
 **********************************************/
void OLED_Init(int fd)
{
	OLED_WR_Byte(0xAE, OLED_CMD); //--display off
	OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
	OLED_WR_Byte(0x10, OLED_CMD); //---set high column address
	OLED_WR_Byte(0x40, OLED_CMD); //--set start line address
	OLED_WR_Byte(0xB0, OLED_CMD); //--set page address
	OLED_WR_Byte(0x81, OLED_CMD); // contract control
	OLED_WR_Byte(0xFF, OLED_CMD); //--128
	OLED_WR_Byte(0xA1, OLED_CMD); // set segment remap
	OLED_WR_Byte(0xA6, OLED_CMD); //--normal / reverse
	OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3F, OLED_CMD); //--1/32 duty
	OLED_WR_Byte(0xC8, OLED_CMD); // Com scan direction
	OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset
	OLED_WR_Byte(0x00, OLED_CMD); //

	OLED_WR_Byte(0xD5, OLED_CMD); // set osc division
	OLED_WR_Byte(0x80, OLED_CMD); //

	OLED_WR_Byte(0xD8, OLED_CMD); // set area color mode off
	OLED_WR_Byte(0x05, OLED_CMD); //

	OLED_WR_Byte(0xD9, OLED_CMD); // Set Pre-Charge Period
	OLED_WR_Byte(0xF1, OLED_CMD); //

	OLED_WR_Byte(0xDA, OLED_CMD); // set com pin configuartion
	OLED_WR_Byte(0x12, OLED_CMD); //

	OLED_WR_Byte(0xDB, OLED_CMD); // set Vcomh
	OLED_WR_Byte(0x30, OLED_CMD); //

	OLED_WR_Byte(0x8D, OLED_CMD); // set charge pump enable
	OLED_WR_Byte(0x14, OLED_CMD); //

	OLED_WR_Byte(0xAF, OLED_CMD); //--turn on oled panel

	s_oled_fd = fd;
}

/**************************************************
 * 函数功能: 设置坐标
 * SSD1306控制芯片内置 128*8=1024字节的GDDRAM, oled
 * 屏幕的有128*64=8192像素, GDDRAM的每1bit对应屏幕
 * 的一个像素. 每一行128像素, 8行为一个page(页), 当
 * 控制芯片处于page模式时, 每写入一个字节, 对应的是
 * 屏幕上指定页的一列的8个像素; 写入时, 字节的高位
 * 写入当前列的底端, 低位写入当前列的顶端.
 *                       --参见SSD1306手册的8.7节
 * 当写入汉字时, 由于现在使用的汉字库时16x16的点阵,
 * 所以一个汉字占两行, 16列
 * ------------------------------------------------
 * @param - x, 列号, 取值范围: 0~127
 * @param - y, page号(或者称行号), 取值范围: 0~7
 * ------------------------------------------------
 * @return - 无
 **************************************************/
void OLED_Set_Pos(u8 x, u8 y)
{
	x = (x % MAX_COLUMN);
	y = (y % PAGE_COUNT);
	OLED_WR_Byte(0xb0 + y, OLED_CMD);                 //设置page号
	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD); //设置列值的高4位
	OLED_WR_Byte((x & 0x0f), OLED_CMD);               //设置列值的低4位
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

//开启OLED显示
void OLED_Display_On()
{
	OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
	OLED_WR_Byte(0X14, OLED_CMD); // DCDC ON
	OLED_WR_Byte(0XAF, OLED_CMD); // DISPLAY ON
}

//关闭OLED显示
void OLED_Display_Off()
{
	OLED_WR_Byte(0X8D, OLED_CMD); // SET DCDC命令
	OLED_WR_Byte(0X10, OLED_CMD); // DCDC OFF
	OLED_WR_Byte(0XAE, OLED_CMD); // DISPLAY OFF
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
void OLED_Set_8x8_Cell(u8 row, u8 col, u8 *data, u8 len)
{
	u8 x = (col % ASCII_CHAR_COUNT_PER_LINE) * ASCII_WIDTH;
	memcpy(&s_OLED_GRAM[row][x], data, len);
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
	OLED_Set_8x8_Cell(row, col, d, ASCII_WIDTH);
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
	memset(&s_OLED_GRAM[row][0], 0, sizeof(s_OLED_GRAM[row]));
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
		OLED_Set_8x8_Cell(row, i, d, ASCII_WIDTH);
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
void OLED_Show8x16Char(u8 row, u8 col, u8 c)
{
	c = c - ' '; //得到字库中的索引

	OLED_Set_8x8_Cell(row, col, (u8*) (F8X16 + c * 16), ASCII_WIDTH);   //显示上半部分
	OLED_Set_8x8_Cell(row + 1, col, (u8*) (F8X16 + c * 16 + PAGE_HEIGHT),
			ASCII_WIDTH); //显示下半部分
}

/******************************************
 * 函数功能: 显示一个16x16的点阵汉字字符
 * ----------------------------------------
 * @param - row, 逻辑行号
 * @param - col, 逻辑列号
 * ----------------------------------------
 * @return - 无
 ******************************************/
void OLED_Show16x16Chinese(u8 row, u8 col, u16 c)
{
	u32 i = 0;
	for (i = 0; i < chzCharCount; i++)
	{
		if (CN16_Msk[i].Index == c)
		{
			OLED_Set_8x8_Cell(row, col, (u8*) (CN16_Msk[i].Msk), ASCII_WIDTH); //显示左上部分
			OLED_Set_8x8_Cell(row, col + 1,
					(u8*) (CN16_Msk[i].Msk + ASCII_WIDTH), ASCII_WIDTH); //显示右上部分
			OLED_Set_8x8_Cell(row + 1, col,
					(u8*) (CN16_Msk[i].Msk + 2 * ASCII_WIDTH), ASCII_WIDTH); //显示左下部分
			OLED_Set_8x8_Cell(row + 1, col + 1,
					(u8*) (CN16_Msk[i].Msk + 3 * ASCII_WIDTH), ASCII_WIDTH); //显示右下部分
		}
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
void OLED_Show16x16Chinese_UTF8(u8 row, u8 col, u32 c)
{
	u32 i = 0;
	for (i = 0; i < chzCharCount; i++)
	{
		if (CN16_Msk[i].unicode == c)
		{
			OLED_Set_8x8_Cell(row, col, (u8*) (CN16_Msk[i].Msk), ASCII_WIDTH); //显示左上部分
			OLED_Set_8x8_Cell(row, col + 1,
					(u8*) (CN16_Msk[i].Msk + ASCII_WIDTH), ASCII_WIDTH); //显示右上部分
			OLED_Set_8x8_Cell(row + 1, col,
					(u8*) (CN16_Msk[i].Msk + 2 * ASCII_WIDTH), ASCII_WIDTH); //显示左下部分
			OLED_Set_8x8_Cell(row + 1, col + 1,
					(u8*) (CN16_Msk[i].Msk + 3 * ASCII_WIDTH), ASCII_WIDTH); //显示右下部分
		}
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
void OLED_DrawBMP(int fd, u8 x0, u8 y0, u8 x1, u8 y1, u8 BMP[])
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
		OLED_Set_Pos(x0, y);

		for (x = x0; x < x1; x++)
		{
			OLED_WR_Byte(BMP[j++], OLED_DATA);
		}
	}
}

/******************************************************
 * 函数功能: 打印一个字符串, 字符串显示大小: 1. 当字符
 * 是ASCII码字符时, 打印 8x16 大小的字符; 2. 当字符时汉
 * 字时, 打印 16x16 大小的字符. 字符占用的行/列, 以8x8像
 * 素为单位, 计算一个逻辑行/逻辑列, 且受参数width制约.
 * !!本函数仅处理 GBK 编码的字符串
 * ---------------------------------------------------
 * @param - row, 逻辑行号, 取值范围 0~7
 * @param - col, 逻辑列号, 取值范围 0~15
 * @param - width, 数据项描述宽度, 以逻辑列为单位, 取值
 *          范围1~15, 即至少1列, 至多15列, 留1列给数值.
 * @param - s, 要显示的字符串
 * ---------------------------------------------------
 * @return - 无
 ******************************************************/
void OLED_Print(u8 row, u8 col, u8 width, char *s)
{
	u32 i;
	u8 delta;
	unsigned short chs = 0;
	u32 length = strlen(s); //取字符串总长
	char *p = s;            //字符串

	u8 curRow = row;
	u8 curCol = col;

	for (i = 0; i < length; i += delta, col += delta)
	{
		if (p[i] <= 127) //小于128是ASCII符号
		{
			delta = 1;
		}
		else if (p[i] > 127) //大于127，为汉字，前后两个组成汉字内码
		{
			chs = (p[i] << 8) | (p[i + 1]); //取汉字的内码
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
			OLED_Show8x16Char(row, col, s[i]);
		}
		else if (2 == delta) //汉字
		{
			OLED_Show16x16Chinese(row, col, chs);
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
void OLED_Print_UTF8(u8 row, u8 col, u8 width, char *s)
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
			OLED_Show8x16Char(curRow, curCol, p[i]);
		}
		else if (2 == delta) //汉字
		{
			OLED_Show16x16Chinese_UTF8(curRow, curCol, unicode);
		}
	}
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
