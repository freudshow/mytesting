#include <stdio.h>
#include <stdint.h>
#include "basedef.h"

void convertBaud(uint8_t databits, uint8_t parity, uint8_t stopbits, uint8_t flowctrl, uint8_t *control_bits)
{
    //设置数据位
    *control_bits &= 0xFC; //先清零
    switch (databits)
    {
        case 5:
            *control_bits |= (0 & 0x03);
            break;
        case 6:
            *control_bits |= (1 & 0x03);
            break;
        case 7:
            *control_bits |= (2 & 0x03);
            break;
        case 8:
            *control_bits |= (3 & 0x03);
            break;
        default:
            DEBUG_TIME_LINE("ERROR: invalid databits %d", databits);
    }

    //设置校验位
    *control_bits &= 0xF3; //先清零
    switch (parity)
    {
        case 0:
            *control_bits |= ((0 << 2) & 0x0C);
            break;
        case 1:
            *control_bits |= ((1 << 2) & 0x0C);
            break;
        case 2:
            *control_bits |= ((2 << 2) & 0x0C);
            break;
        default:
            DEBUG_TIME_LINE("ERROR: invalid parity %d", parity);
    }

    //设置停止位
    *control_bits &= 0xEF; //先清零
    switch (stopbits)
    {
        case 1:
            *control_bits |= ((0 << 4) & 0x10);
            break;
        case 2:
            *control_bits |= ((1 << 4) & 0x10);
            break;
        default:
            DEBUG_TIME_LINE("ERROR: invalid stopbits %d", stopbits);
    }

    //设置流控
    *control_bits &= 0x9F; //先清零
    switch (flowctrl)
    {
        case 0:
            *control_bits |= ((0 << 5) & 0x60);
            break;
        case 1:
            *control_bits |= ((1 << 5) & 0x60);
            break;
        case 2:
            *control_bits |= ((2 << 5) & 0x60);
            break;
        default:
            DEBUG_TIME_LINE("ERROR: invalid flowctrl %d", flowctrl);
    }
}

void getBinary(uint8_t byte, char *buffer)
{
    buffer[0] = ((byte >> 7) & 0x01) ? '1' : '0';
    buffer[1] = ' ';
    buffer[2] = ((byte >> 6) & 0x01) ? '1' : '0';
    buffer[3] = ((byte >> 5) & 0x01) ? '1' : '0';
    buffer[4] = ' ';
    buffer[5] = ((byte >> 4) & 0x01) ? '1' : '0';
    buffer[6] = ' ';
    buffer[7] = ((byte >> 3) & 0x01) ? '1' : '0';
    buffer[8] = ((byte >> 2) & 0x01) ? '1' : '0';
    buffer[9] = ' ';
    buffer[10] = ((byte >> 1) & 0x01) ? '1' : '0';
    buffer[11] = ((byte >> 0) & 0x01) ? '1' : '0';

    buffer[12] = '\0';
}

void testbaud(void)
{
    uint8_t control_bits = 0;
    uint8_t databits[] = { 5, 6, 7, 8 };
    uint8_t parity[] = { 0, 1, 2 };
    uint8_t stopbits[] = { 1, 2 };
    uint8_t flowctrl[] = { 0, 1, 2 };

    char buffer[256];

    for (int i = 0; i < sizeof(databits); i++)
    {
        for (int j = 0; j < sizeof(parity); j++)
        {
            for (int k = 0; k < sizeof(stopbits); k++)
            {
                for (int l = 0; l < sizeof(flowctrl); l++)
                {
                    convertBaud(databits[i], parity[j], stopbits[k], flowctrl[l], &control_bits);
                    getBinary(control_bits, buffer);
                    DEBUG_TIME_LINE("databits: %d, parity: %d, stopbits: %d, flowctrl: %d => control_bits: 0x%02X, %s",
                            databits[i], parity[j], stopbits[k], flowctrl[l], control_bits, buffer);

                    // Data bits check
                    if (databits[i] == 5)
                    {
                        if ((control_bits & 0x03) == 0)
                        {
                            DEBUG_TIME_LINE("Data Bits passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Data Bits failed");
                        }
                    }
                    else if (databits[i] == 6)
                    {
                        if ((control_bits & 0x03) == 1)
                        {
                            DEBUG_TIME_LINE("Data Bits passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Data Bits failed");
                        }
                    }
                    else if (databits[i] == 7)
                    {
                        if ((control_bits & 0x03) == 2)
                        {
                            DEBUG_TIME_LINE("Data Bits passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Data Bits failed");
                        }
                    }
                    else if (databits[i] == 8)
                    {
                        if ((control_bits & 0x03) == 3)
                        {
                            DEBUG_TIME_LINE("Data Bits passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Data Bits failed");
                        }
                    }

                    // Parity check
                    if (parity[j] == 0)
                    {
                        if (((control_bits & 0x0C) >> 2) == 0)
                        {
                            DEBUG_TIME_LINE("Parity passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Parity failed");
                        }
                    }
                    else if (parity[j] == 1)
                    {
                        if (((control_bits & 0x0C) >> 2) == 1)
                        {
                            DEBUG_TIME_LINE("Parity passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Parity failed");
                        }
                    }
                    else if (parity[j] == 2)
                    {
                        if (((control_bits & 0x0C) >> 2) == 2)
                        {
                            DEBUG_TIME_LINE("Parity passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Parity failed");
                        }
                    }

                    // Stop bits check
                    if (stopbits[k] == 1)
                    {
                        if (((control_bits & 0x10) >> 4) == 0)
                        {
                            DEBUG_TIME_LINE("Stop Bits passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Stop Bits failed");
                        }
                    }
                    else if (stopbits[k] == 2)
                    {
                        if (((control_bits & 0x10) >> 4) == 1)
                        {
                            DEBUG_TIME_LINE("Stop Bits passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Stop Bits failed");
                        }
                    }

                    // Flow control check
                    if (flowctrl[l] == 0)
                    {
                        if (((control_bits & 0x60) >> 5) == 0)
                        {
                            DEBUG_TIME_LINE("Flow Control passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Flow Control failed");
                        }
                    }
                    else if (flowctrl[l] == 1)
                    {
                        if (((control_bits & 0x60) >> 5) == 1)
                        {
                            DEBUG_TIME_LINE("Flow Control passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Flow Control failed");
                        }
                    }
                    else if (flowctrl[l] == 2)
                    {
                        if (((control_bits & 0x60) >> 5) == 2)
                        {
                            DEBUG_TIME_LINE("Flow Control passed");
                        }
                        else
                        {
                            DEBUG_TIME_LINE("Flow Control failed");
                        }
                    }
                }
            }
        }
    }
}
