/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2022/09/01
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*
 * @Note
 * This example demonstrates the process of enumerating the keyboard and mouse 
 * by a USB host and obtaining data based on the polling time of the input endpoints 
 * of the keyboard and mouse. 
 * The USBFS port also supports enumeration of keyboard and mouse attached at tier
 * level 2(Hub 1).
*/

/*
 * @Note
 * Please select the corresponding macro definition (CH32V30x_D8C/CH32V30x_D8)
 * and startup_xxx.s file according to the chip model, otherwise the example may be abnormal.
 * In addition, when the system clock is selected as the USBFS clock source, only 144MHz/96MHz/48MHz
 * are supported.
 */

/*******************************************************************************/
/* Header Files */
#include "usb_host_config.h"
#include "submouse_serial.h"
#include "usbd_hs_mouse.h"

static uint8_t MergeMouseReports( MouseReport_t *out,
                                  const MouseReport_t *phy,
                                  uint8_t phy_valid,
                                  const MouseReport_t *serial,
                                  uint8_t serial_valid )
{
    if( !phy_valid && !serial_valid )
    {
        return 0;
    }

    out->dx = 0;
    out->dy = 0;
    out->wheel = 0;
    out->buttons = 0;
    out->valid = 1;

    if( phy_valid )
    {
        out->dx += phy->dx;
        out->dy += phy->dy;
        out->wheel += phy->wheel;
        out->buttons |= phy->buttons;
    }

    if( serial_valid )
    {
        out->dx += serial->dx;
        out->dy += serial->dy;
        out->wheel += serial->wheel;
        out->buttons |= serial->buttons;
    }

    return 1;
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main( void )
{
    /* Initialize system configuration */
    SystemCoreClockUpdate( );
    Delay_Init( );
    USART_Printf_Init( 115200 );

    printf( "SystemClk:%d\r\n", SystemCoreClock );
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
    printf( "USBFS official HOST_KM PA2 diag\r\n" );
    printf( "HOST_KM_BRIDGE mouse bridge\r\n" );
    printf( "LOW direct mode: PRE=0\r\n" );
    printf( "USBFS Host + USART1 sub input + USBHS HID output\r\n" );
    printf( "Wire USB-A: red=5V black=GND white=PA11(D-) green=PA12(D+)\r\n" );

    /* Initialize TIM3 */
    TIM3_Init( 9, SystemCoreClock / 10000 - 1 );
    printf( "TIM3 Init OK!\r\n" );

    /* Initialize USBHS host */
    /* Note: Only CH32V305/CH32V307 support USB high-speed port. */
#if DEF_USBHS_PORT_EN
    printf( "USBHS Host Init\r\n" );
    USBHS_RCC_Init( );
    USBHS_Host_Init( ENABLE );
    memset( &RootHubDev[ DEF_USBHS_PORT_INDEX ].bStatus, 0, sizeof( ROOT_HUB_DEVICE ) );
    memset( &HostCtl[ DEF_USBHS_PORT_INDEX * DEF_ONE_USB_SUP_DEV_TOTAL ].InterfaceNum, 0, DEF_ONE_USB_SUP_DEV_TOTAL * sizeof( HOST_CTL ) );
#endif

    /* Initialize USBFS host */
#if DEF_USBFS_PORT_EN
    printf( "USBFS Host Init\r\n" );
    USBFS_RCC_Init( );
    USBFS_Host_Init( ENABLE );
    memset( &RootHubDev[ DEF_USBFS_PORT_INDEX ].bStatus, 0, sizeof( ROOT_HUB_DEVICE ) );
    memset( &HostCtl[ DEF_USBFS_PORT_INDEX * DEF_ONE_USB_SUP_DEV_TOTAL ].InterfaceNum, 0, DEF_ONE_USB_SUP_DEV_TOTAL * sizeof( HOST_CTL ) );
#endif

    SubMouseSerial_Init( 921600 );
    USBHSD_Mouse_Init( );
    
    while( 1 )
    {
        static uint16_t alive_ms;
        MouseReport_t phy_report;
        MouseReport_t serial_report;
        MouseReport_t out_report;
        USBHSD_MouseDiag_t usbhs_diag;
        uint8_t phy_valid;
        uint8_t serial_valid;

        USBH_MainDeal( );
        SubMouseSerial_Task( );

        phy_valid = USBH_MouseBridge_ReadReport( &phy_report );
        serial_valid = SubMouseSerial_ReadReport( &serial_report );
        if( MergeMouseReports( &out_report, &phy_report, phy_valid, &serial_report, serial_valid ) )
        {
            if( USBHSD_Mouse_SendReport( &out_report ) )
            {
                printf( "USBHS mouse sent dx:%d dy:%d btn:%02x wh:%d src:%c%c\r\n",
                        out_report.dx,
                        out_report.dy,
                        out_report.buttons,
                        out_report.wheel,
                        phy_valid ? 'P' : '-',
                        serial_valid ? 'S' : '-' );
            }
            else
            {
                printf( "USBHS mouse pending cfg:%u dx:%d dy:%d btn:%02x\r\n",
                        USBHSD_Mouse_IsConfigured( ),
                        out_report.dx,
                        out_report.dy,
                        out_report.buttons );
            }
        }

        Delay_Ms( 1 );
        alive_ms++;
        if( alive_ms >= 1000 )
        {
            alive_ms = 0;
            SubMouseSerial_SendAlive( );
            USBHSD_Mouse_GetDiag( &usbhs_diag );
            printf( "STAT usbhs_cfg:%u phy_cfg:%u phy_rpt:%lu\r\n",
                    usbhs_diag.config,
                    USBH_MouseBridge_IsConfigured( ),
                    USBH_MouseBridge_ReportCount( ) );
        }
    }
}



