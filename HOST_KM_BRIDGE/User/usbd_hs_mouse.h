/********************************** (C) COPYRIGHT *******************************
 * USBHS HID mouse device on USB2/PB6/PB7.
*******************************************************************************/
#ifndef __USBD_HS_MOUSE_H
#define __USBD_HS_MOUSE_H

#include "mouse_report.h"

typedef struct
{
    uint32_t reset_count;
    uint32_t setup_count;
    uint32_t set_addr_count;
    uint32_t set_config_count;
    uint8_t addr;
    uint8_t config;
    uint8_t last_req_type;
    uint8_t last_req;
    uint16_t last_value;
    uint16_t last_index;
    uint16_t last_length;
    uint8_t last_intflag;
    uint8_t last_intst;
} USBHSD_MouseDiag_t;

void USBHSD_Mouse_Init(void);
uint8_t USBHSD_Mouse_IsConfigured(void);
uint8_t USBHSD_Mouse_SendReport(const MouseReport_t *report);
void USBHSD_Mouse_GetDiag(USBHSD_MouseDiag_t *diag);

#endif
