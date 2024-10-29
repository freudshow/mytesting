#include "oled.h"
#include "oled_gui.h"

void OLED_GUI_DrawPoint(oledGuiPoint_s *pPoint, u8 **GRAM)
{
    u8 row, m, n;

    pPoint->x = (pPoint->x % MAX_COLUMN);
    pPoint->y = (pPoint->y % MAX_ROW);
    row = (pPoint->y / PAGE_HEIGHT); //像素位于哪一行，1行=8个纵向像素
    m = (pPoint->y % PAGE_HEIGHT); //像素位于一列中的哪一行，从上到下开始数
    n = 1 << m; //像素的值
    if (pPoint->OnOff) //如果是点亮，则将这一像素对应的位, 置1
    {
        GRAM[row][pPoint->x] |= n;
    }
    else   //如果是熄灭，则将这一像素对应的位, 置0
    {
        GRAM[row][pPoint->x] &= (~(1 << m));
    }
}

void OLED_clearRectangle(u8 top, u8 bottom, u8 left, u8 right, u8 **GRAM)
{
    oledGuiPoint_s point = { 0 };
    for (point.y = top; point.y < bottom; point.y++)
    {
        for (point.x = left; point.x < right; point.x++)
        {
            OLED_GUI_DrawPoint(&point, GRAM);
        }
    }
}
