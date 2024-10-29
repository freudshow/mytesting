#ifndef OLED_GUI_H

#include "basedef.h"

typedef struct oledGuiPoint {
    u8 x;       //column, interval [0-127]
    u8 y;       //row, interval [0-63]
    u8 OnOff;   //1 - on; 0 or other - off;
} oledGuiPoint_s;

typedef struct oledGuiText {
        oledGuiPoint_s point;
        char *text;
        u8 textLen;
        u8 OnOff;
} oledGuiText_s;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

extern void OLED_GUI_DrawPoint(oledGuiPoint_s *pPoint, u8 **GRAM);
extern void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2, u8 mode, u8 **GRAM);
extern void OLED_DrawCircle(u8 x, u8 y, u8 r, u8 **GRAM);
extern void OLED_clearRectangle(u8 top, u8 bottom, u8 left, u8 right, u8 **GRAM);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus extern "C"*/

#endif//OLED_GUI_H
