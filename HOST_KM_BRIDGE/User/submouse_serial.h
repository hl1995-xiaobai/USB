/********************************** (C) COPYRIGHT *******************************
 * Secondary-machine mouse frame input over USART3.
*******************************************************************************/
#ifndef __SUBMOUSE_SERIAL_H
#define __SUBMOUSE_SERIAL_H

#include <stdint.h>
#include "mouse_report.h"

void SubMouseSerial_Init(uint32_t baudrate);
void SubMouseSerial_Task(void);
void SubMouseSerial_SendAlive(void);
uint8_t SubMouseSerial_ReadReport(MouseReport_t *report);

#endif
