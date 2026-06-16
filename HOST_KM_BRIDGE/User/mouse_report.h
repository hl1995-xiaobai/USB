/********************************** (C) COPYRIGHT *******************************
 * Mouse report shared by serial input and USB HID output.
*******************************************************************************/
#ifndef __MOUSE_REPORT_H
#define __MOUSE_REPORT_H

#include <stdint.h>

typedef struct
{
    int16_t dx;
    int16_t dy;
    int8_t wheel;
    uint8_t buttons;
    uint8_t valid;
} MouseReport_t;

#endif
