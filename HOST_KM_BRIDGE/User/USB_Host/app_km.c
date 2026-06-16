/********************************** (C) COPYRIGHT  *******************************
 * File Name          : app_km.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2022/09/01
 * Description        : The USB host operates the keyboard and mouse.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/


/********************************************************************************/
/* Header File */
#include "usb_host_config.h"

/*******************************************************************************/
/* Variable Definition */
uint8_t  DevDesc_Buf[ 18 ];                                                     // Device Descriptor Buffer
uint8_t  Com_Buf[ DEF_COM_BUF_LEN ];                                            // General Buffer
struct   _ROOT_HUB_DEVICE RootHubDev[ DEF_TOTAL_ROOT_HUB ];
struct   __HOST_CTL HostCtl[ DEF_TOTAL_ROOT_HUB * DEF_ONE_USB_SUP_DEV_TOTAL ];
static MouseReport_t Bridge_MouseReport;
static uint8_t Bridge_MouseReportValid;
static uint32_t Bridge_MouseReportCount;

typedef struct
{
    uint8_t valid;
    uint8_t report_id;
    uint8_t has_report_id;
    uint16_t button_offset;
    uint8_t button_count;
    uint16_t x_offset;
    uint8_t x_size;
    uint16_t y_offset;
    uint8_t y_size;
    uint16_t wheel_offset;
    uint8_t wheel_size;
} HID_MOUSE_REPORT_MAP;

static HID_MOUSE_REPORT_MAP Bridge_MouseMap[ DEF_TOTAL_ROOT_HUB * DEF_ONE_USB_SUP_DEV_TOTAL ][ DEF_INTERFACE_NUM_MAX ];

/*******************************************************************************/
/* Interrupt Function Declaration */
void TIM3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

static int32_t USBH_MouseBridge_ReadBits( const uint8_t *pbuf, uint16_t len, uint16_t bit_offset, uint8_t bit_size )
{
    uint8_t i;
    uint32_t value = 0;

    if( bit_size == 0 )
    {
        return 0;
    }

    for( i = 0; i < bit_size; i++ )
    {
        uint16_t bit = (uint16_t)bit_offset + i;
        if( ( bit / 8 ) >= len )
        {
            break;
        }
        if( pbuf[ bit / 8 ] & ( 1 << ( bit & 0x07 ) ) )
        {
            value |= (uint32_t)1 << i;
        }
    }

    return (int32_t)value;
}

static int16_t USBH_MouseBridge_ReadSignedBits( const uint8_t *pbuf, uint16_t len, uint16_t bit_offset, uint8_t bit_size )
{
    int32_t value;

    if( bit_size == 0 )
    {
        return 0;
    }

    value = USBH_MouseBridge_ReadBits( pbuf, len, bit_offset, bit_size );
    if( ( bit_size < 32 ) && ( value & ( (int32_t)1 << ( bit_size - 1 ) ) ) )
    {
        value -= (int32_t)1 << bit_size;
    }

    if( value > 32767 )
    {
        value = 32767;
    }
    else if( value < -32768 )
    {
        value = -32768;
    }

    return (int16_t)value;
}

static uint8_t USBH_MouseBridge_ParseMappedReport( const HID_MOUSE_REPORT_MAP *map, uint8_t *pbuf, uint16_t len )
{
    uint16_t data_bit_base = 0;

    if( ( map == 0 ) || ( map->valid == 0 ) )
    {
        return 0;
    }

    if( map->has_report_id )
    {
        if( ( len == 0 ) || ( pbuf[ 0 ] != map->report_id ) )
        {
            return 0;
        }
        data_bit_base = 8;
    }

    Bridge_MouseReport.buttons = USBH_MouseBridge_ReadBits( pbuf, len, data_bit_base + map->button_offset, map->button_count ) & 0x07;
    Bridge_MouseReport.dx = USBH_MouseBridge_ReadSignedBits( pbuf, len, data_bit_base + map->x_offset, map->x_size );
    Bridge_MouseReport.dy = USBH_MouseBridge_ReadSignedBits( pbuf, len, data_bit_base + map->y_offset, map->y_size );
    Bridge_MouseReport.wheel = map->wheel_size ? (int8_t)USBH_MouseBridge_ReadSignedBits( pbuf, len, data_bit_base + map->wheel_offset, map->wheel_size ) : 0;
    return 1;
}

static void USBH_MouseBridge_ParseFallbackReport( uint8_t *pbuf, uint16_t len )
{
    if( ( len >= 6 ) && ( pbuf[ 0 ] == 0x01 ) )
    {
        uint16_t raw_x;
        uint16_t raw_y;

        raw_x = (uint16_t)pbuf[ 2 ] | ( (uint16_t)( pbuf[ 3 ] & 0x0F ) << 8 );
        raw_y = ( (uint16_t)( pbuf[ 3 ] >> 4 ) & 0x0F ) | ( (uint16_t)pbuf[ 4 ] << 4 );

        Bridge_MouseReport.buttons = pbuf[ 1 ] & 0x07;
        Bridge_MouseReport.dx = USBH_MouseBridge_ReadSignedBits( (const uint8_t *)&raw_x, 2, 0, 12 );
        Bridge_MouseReport.dy = USBH_MouseBridge_ReadSignedBits( (const uint8_t *)&raw_y, 2, 0, 12 );
        Bridge_MouseReport.wheel = 0;
    }
    else
    {
        Bridge_MouseReport.buttons = pbuf[ 0 ] & 0x07;
        Bridge_MouseReport.dx = (int8_t)pbuf[ 1 ];
        Bridge_MouseReport.dy = (int8_t)pbuf[ 2 ];
        Bridge_MouseReport.wheel = ( len >= 4 ) ? (int8_t)pbuf[ 3 ] : 0;
    }
}

static void USBH_MouseBridge_StoreReport( uint8_t index, uint8_t intf_num, uint8_t *pbuf, uint16_t len )
{
    const HID_MOUSE_REPORT_MAP *map;
    uint8_t mapped;

    if( ( len < 3 ) || ( index >= ( DEF_TOTAL_ROOT_HUB * DEF_ONE_USB_SUP_DEV_TOTAL ) ) || ( intf_num >= DEF_INTERFACE_NUM_MAX ) )
    {
        return;
    }

    map = &Bridge_MouseMap[ index ][ intf_num ];
    mapped = USBH_MouseBridge_ParseMappedReport( map, pbuf, len );
    if( !mapped )
    {
        USBH_MouseBridge_ParseFallbackReport( pbuf, len );
    }

    Bridge_MouseReport.valid = 1;
    Bridge_MouseReportValid = 1;
    Bridge_MouseReportCount++;

    DUG_PRINTF( "PHY mouse raw len:%u map:%u id:%02x %02x %02x %02x %02x %02x %02x %02x -> dx:%d dy:%d btn:%02x wh:%d\r\n",
                len,
                mapped,
                map->has_report_id ? map->report_id : 0,
                pbuf[ 0 ],
                pbuf[ 1 ],
                pbuf[ 2 ],
                ( len >= 4 ) ? pbuf[ 3 ] : 0,
                ( len >= 5 ) ? pbuf[ 4 ] : 0,
                ( len >= 6 ) ? pbuf[ 5 ] : 0,
                ( len >= 7 ) ? pbuf[ 6 ] : 0,
                Bridge_MouseReport.dx,
                Bridge_MouseReport.dy,
                Bridge_MouseReport.buttons,
                Bridge_MouseReport.wheel );
}

static int32_t USBH_MouseBridge_ReadItemData( const uint8_t *pbuf, uint16_t len, uint16_t *pos, uint8_t item_size )
{
    uint8_t i;
    uint8_t bytes;
    uint32_t value = 0;

    if( item_size == 3 )
    {
        bytes = 4;
    }
    else
    {
        bytes = item_size;
    }

    for( i = 0; i < bytes; i++ )
    {
        if( *pos < len )
        {
            value |= (uint32_t)pbuf[ *pos ] << ( i * 8 );
        }
        ( *pos )++;
    }

    if( bytes == 1 )
    {
        return (int8_t)value;
    }
    if( bytes == 2 )
    {
        return (int16_t)value;
    }

    return (int32_t)value;
}

static void USBH_MouseBridge_AnalyzeMouseReportDesc( uint8_t index, uint8_t intf_num, const uint8_t *pbuf, uint16_t len )
{
    HID_MOUSE_REPORT_MAP candidate;
    HID_MOUSE_REPORT_MAP best;
    uint8_t usage_page = 0;
    uint8_t report_size = 0;
    uint8_t report_count = 0;
    int32_t logical_min = 0;
    int32_t logical_max = 0;
    uint8_t report_id = 0;
    uint16_t report_bit_offset = 0;
    uint8_t local_usages[ 16 ];
    uint8_t usage_count = 0;
    uint8_t usage_min = 0;
    uint8_t usage_max = 0;
    uint8_t has_usage_range = 0;
    uint16_t pos = 0;

    if( ( index >= ( DEF_TOTAL_ROOT_HUB * DEF_ONE_USB_SUP_DEV_TOTAL ) ) || ( intf_num >= DEF_INTERFACE_NUM_MAX ) )
    {
        return;
    }

    memset( &candidate, 0, sizeof( candidate ) );
    memset( &best, 0, sizeof( best ) );

    while( pos < len )
    {
        uint8_t prefix = pbuf[ pos++ ];
        uint8_t item_size = prefix & 0x03;
        uint8_t item_type = prefix & 0x0C;
        uint8_t item_tag = prefix & 0xF0;
        uint8_t item_pos = pos;
        int32_t item_value;

        if( prefix == 0xFE )
        {
            if( pos + 1 >= len )
            {
                break;
            }
            pos += (uint16_t)pbuf[ pos ] + 2;
            usage_count = 0;
            has_usage_range = 0;
            continue;
        }

        item_value = USBH_MouseBridge_ReadItemData( pbuf, len, &pos, item_size );

        switch( item_type )
        {
            case 0x00: /* Main */
                if( item_tag == 0x80 ) /* Input */
                {
                    uint8_t is_constant = item_value & 0x01;
                    uint8_t is_relative = item_value & 0x04;
                    uint8_t field;

                    if( !is_constant )
                    {
                        for( field = 0; field < report_count; field++ )
                        {
                            uint8_t usage = 0;
                            uint16_t field_offset = report_bit_offset + (uint16_t)field * report_size;

                            if( field < usage_count )
                            {
                                usage = local_usages[ field ];
                            }
                            else if( has_usage_range && ( ( usage_min + field ) <= usage_max ) )
                            {
                                usage = usage_min + field;
                            }

                            if( usage_page == 0x09 )
                            {
                                if( candidate.button_count == 0 )
                                {
                                    candidate.button_offset = field_offset;
                                }
                                if( candidate.button_count < 8 )
                                {
                                    candidate.button_count++;
                                }
                            }
                            else if( ( usage_page == 0x01 ) && is_relative )
                            {
                                if( usage == 0x30 )
                                {
                                    candidate.x_offset = field_offset;
                                    candidate.x_size = report_size;
                                }
                                else if( usage == 0x31 )
                                {
                                    candidate.y_offset = field_offset;
                                    candidate.y_size = report_size;
                                }
                                else if( usage == 0x38 )
                                {
                                    candidate.wheel_offset = field_offset;
                                    candidate.wheel_size = report_size;
                                }
                            }
                        }
                    }

                    report_bit_offset += (uint16_t)report_size * report_count;
                    usage_count = 0;
                    has_usage_range = 0;
                }
                else if( ( item_tag == 0xA0 ) || ( item_tag == 0xC0 ) ) /* Collection / End Collection */
                {
                    usage_count = 0;
                    has_usage_range = 0;
                }
                break;

            case 0x04: /* Global */
                if( item_tag == 0x00 ) /* Usage Page */
                {
                    usage_page = (uint8_t)item_value;
                }
                else if( item_tag == 0x10 ) /* Logical Minimum */
                {
                    logical_min = item_value;
                    (void)logical_min;
                }
                else if( item_tag == 0x20 ) /* Logical Maximum */
                {
                    logical_max = item_value;
                    (void)logical_max;
                }
                else if( item_tag == 0x70 ) /* Report Size */
                {
                    report_size = (uint8_t)item_value;
                }
                else if( item_tag == 0x80 ) /* Report ID */
                {
                    if( candidate.x_size && candidate.y_size && !best.valid )
                    {
                        candidate.valid = 1;
                        best = candidate;
                    }
                    report_id = (uint8_t)item_value;
                    memset( &candidate, 0, sizeof( candidate ) );
                    candidate.report_id = report_id;
                    candidate.has_report_id = 1;
                    report_bit_offset = 0;
                }
                else if( item_tag == 0x90 ) /* Report Count */
                {
                    report_count = (uint8_t)item_value;
                }
                break;

            case 0x08: /* Local */
                if( item_tag == 0x00 ) /* Usage */
                {
                    if( usage_count < sizeof( local_usages ) )
                    {
                        local_usages[ usage_count++ ] = pbuf[ item_pos ];
                    }
                }
                else if( item_tag == 0x10 ) /* Usage Minimum */
                {
                    usage_min = (uint8_t)item_value;
                    has_usage_range = 1;
                }
                else if( item_tag == 0x20 ) /* Usage Maximum */
                {
                    usage_max = (uint8_t)item_value;
                    has_usage_range = 1;
                }
                break;

            default:
                break;
        }
    }

    if( candidate.x_size && candidate.y_size && !best.valid )
    {
        candidate.valid = 1;
        best = candidate;
    }

    Bridge_MouseMap[ index ][ intf_num ] = best;
    DUG_PRINTF( "PHY mouse map intf:%u valid:%u id:%02x btn:%u@%u x:%u@%u y:%u@%u wh:%u@%u\r\n",
                intf_num,
                best.valid,
                best.has_report_id ? best.report_id : 0,
                best.button_count,
                best.button_offset,
                best.x_size,
                best.x_offset,
                best.y_size,
                best.y_offset,
                best.wheel_size,
                best.wheel_offset );
}

uint8_t USBH_MouseBridge_ReadReport( MouseReport_t *report )
{
    if( Bridge_MouseReportValid == 0 )
    {
        return 0;
    }

    *report = Bridge_MouseReport;
    Bridge_MouseReport.dx = 0;
    Bridge_MouseReport.dy = 0;
    Bridge_MouseReport.wheel = 0;
    Bridge_MouseReport.valid = 0;
    Bridge_MouseReportValid = 0;
    return 1;
}

uint8_t USBH_MouseBridge_IsConfigured( void )
{
    uint8_t usb_port;
    uint8_t index;
    uint8_t intf_num;

    for( usb_port = 0; usb_port < DEF_TOTAL_ROOT_HUB; usb_port++ )
    {
        if( ( RootHubDev[ usb_port ].bStatus >= ROOT_DEV_SUCCESS ) &&
            ( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HID ) )
        {
            index = RootHubDev[ usb_port ].DeviceIndex;
            for( intf_num = 0; intf_num < HostCtl[ index ].InterfaceNum; intf_num++ )
            {
                if( HostCtl[ index ].Interface[ intf_num ].Type == DEC_MOUSE )
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

uint32_t USBH_MouseBridge_ReportCount( void )
{
    return Bridge_MouseReportCount;
}

static void USBH_PrintFsDiag( const char *tag )
{
#if DEF_USBFS_PORT_EN
    uint8_t hres = USBFSH_LastIntSt & USBFS_UIS_H_RES_MASK;
    uint8_t dp = ( USBFSH->HOST_CTRL & USBFS_UH_DP_PIN ) ? 1 : 0;
    uint8_t dm = ( USBFSH->HOST_CTRL & USBFS_UH_DM_PIN ) ? 1 : 0;

    DUG_PRINTF( "FS %s sp:%s(%u) dp:%u dm:%u attach:%u low:%u pre:%u\r\n",
                tag,
                ( RootHubDev[ DEF_USBFS_PORT_INDEX ].bSpeed == USB_FULL_SPEED ) ? "FULL" :
                ( RootHubDev[ DEF_USBFS_PORT_INDEX ].bSpeed == USB_LOW_SPEED ) ? "LOW" : "UNK",
                RootHubDev[ DEF_USBFS_PORT_INDEX ].bSpeed,
                dp,
                dm,
                ( USBFSH->MIS_ST & USBFS_UMS_DEV_ATTACH ) ? 1 : 0,
                ( USBFSH->HOST_CTRL & USBFS_UH_LOW_SPEED ) ? 1 : 0,
                ( USBFSH->HOST_SETUP & USBFS_UH_PRE_PID_EN ) ? 1 : 0 );
    DUG_PRINTF( "FS regs mis:%02x hc:%02x hs:%04x ifg:%02x ist:%02x rx:%u\r\n",
                USBFSH->MIS_ST,
                USBFSH->HOST_CTRL,
                USBFSH->HOST_SETUP,
                USBFSH->INT_FG,
                USBFSH->INT_ST,
                USBFSH->RX_LEN );
    DUG_PRINTF( "FS last pid:%02x tog:%02x ret:%02x ifg:%02x ist:%02x hres:%02x nak:%u togok:%u rx:%u wait:%u\r\n",
                USBFSH_LastPid,
                USBFSH_LastTog,
                USBFSH_LastRet,
                USBFSH_LastIntFg,
                USBFSH_LastIntSt,
                hres,
                ( USBFSH_LastIntSt & USBFS_UIS_IS_NAK ) ? 1 : 0,
                ( USBFSH_LastIntSt & USBFS_UIS_TOG_OK ) ? 1 : 0,
                USBFSH_LastRxLen,
                USBFSH_LastWaitLeft );
#else
    (void)tag;
#endif
}

/*********************************************************************
 * @fn      TIM3_Init
 *
 * @brief   Initialize timer3 for getting keyboard and mouse data.
 *
 * @param   arr - The specific period value.
 *          psc - The specifies prescaler value.
 *
 * @return  none
 */
void TIM3_Init( uint16_t arr, uint16_t psc )
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = { 0 };
    NVIC_InitTypeDef NVIC_InitStructure = { 0 };

    /* Enable timer3 clock */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM3, ENABLE );

    /* Initialize timer3 */
    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit( TIM3, &TIM_TimeBaseStructure );

    /* Enable updating timer3 interrupt */
    TIM_ITConfig( TIM3, TIM_IT_Update, ENABLE );

    /* Configure timer3 interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    /* Enable timer3 */
    TIM_Cmd( TIM3, ENABLE );

    /* Enable timer3 interrupt */
    NVIC_EnableIRQ( TIM3_IRQn );
}

/*********************************************************************
 * @fn      TIM3_IRQHandler
 *
 * @brief   This function handles TIM3 global interrupt request.
 *
 * @return  none
 */
void TIM3_IRQHandler( void )
{
    uint8_t usb_port;
    uint8_t hub_port;
    uint8_t index;
    uint8_t intf_num, in_num;

    if( TIM_GetITStatus( TIM3, TIM_IT_Update ) != RESET )
    {
        /* Clear interrupt flag */
        TIM_ClearITPendingBit( TIM3, TIM_IT_Update );

        /* USB HID Device Input Endpoint Timing */
        for( usb_port = 0; usb_port < DEF_TOTAL_ROOT_HUB; usb_port++ )
        {
            if( RootHubDev[ usb_port ].bStatus >= ROOT_DEV_SUCCESS )
            {
                index = RootHubDev[ usb_port ].DeviceIndex;
                if( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HID )
                {
                    for( intf_num = 0; intf_num < HostCtl[ index ].InterfaceNum; intf_num++ )
                    {
                        for( in_num = 0; in_num < HostCtl[ index ].Interface[ intf_num ].InEndpNum; in_num++ )
                        {
                            HostCtl[ index ].Interface[ intf_num ].InEndpTimeCount[ in_num ]++;
                        }
                    }
                }
                else if( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HUB )
                {
                    HostCtl[ index ].Interface[ 0 ].InEndpTimeCount[ 0 ]++;
                    for( hub_port = 0; hub_port < RootHubDev[ usb_port ].bPortNum; hub_port++ )
                    {
                        if( RootHubDev[ usb_port ].Device[ hub_port ].bStatus >= ROOT_DEV_SUCCESS )
                        {
                            index = RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex;

                            if( RootHubDev[ usb_port ].Device[ hub_port ].bType == USB_DEV_CLASS_HID )
                            {
                                for( intf_num = 0; intf_num < HostCtl[ index ].InterfaceNum; intf_num++ )
                                {
                                    for( in_num = 0; in_num < HostCtl[ index ].Interface[ intf_num ].InEndpNum; in_num++ )
                                    {
                                        HostCtl[ index ].Interface[ intf_num ].InEndpTimeCount[ in_num ]++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      USBH_CheckRootHubPortStatus
 *
 * @brief   Check status of USB port.
 *
 * @para    index: USB host port
 *
 * @return  The current status of the port.
 */
uint8_t USBH_CheckRootHubPortStatus( uint8_t usb_port )
{
    uint8_t s = ERR_USB_UNSUPPORT;
    
    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_CheckRootHubPortStatus( RootHubDev[ usb_port ].bStatus );
#endif            
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN     
        s = USBHSH_CheckRootHubPortStatus( RootHubDev[ usb_port ].bStatus );
#endif      
    }
    
    return s;
}

/*********************************************************************
 * @fn      USBH_ResetRootHubPort
 *
 * @brief   Reset USB port.
 *
 * @para    index: USB host port
 *          mod: Reset host port operating mode.
 *               0 -> reset and wait end
 *               1 -> begin reset
 *               2 -> end reset
 *
 * @return  none
 */
void USBH_ResetRootHubPort( uint8_t usb_port, uint8_t mode )
{
    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        USBFSH_ResetRootHubPort( mode );
#endif
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX ) 
    {
#if DEF_USBHS_PORT_EN
        USBHSH_ResetRootHubPort( mode );
#endif
    }
}

/*********************************************************************
 * @fn      USBH_EnableRootHubPort
 *
 * @brief   Enable USB host port.
 *
 * @para    index: USB host port
 *
 * @return  none
 */
uint8_t USBH_EnableRootHubPort( uint8_t usb_port )
{
    uint8_t s = ERR_USB_UNSUPPORT;
    
    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_EnableRootHubPort( &RootHubDev[ usb_port ].bSpeed );
#endif            
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN     
        s = USBHSH_EnableRootHubPort( &RootHubDev[ usb_port ].bSpeed );
#endif      
    }
   
    return s;
}

/*********************************************************************
 * @fn      USBH_GetDeviceDescr
 *
 * @brief   Get the device descriptor of the USB device.
 *
 * @para    index: USB host port
 *
 * @return  none
 */
uint8_t USBH_GetDeviceDescr( uint8_t usb_port )
{
    uint8_t s = ERR_USB_UNSUPPORT;
    
    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_GetDeviceDescr( &RootHubDev[ usb_port ].bEp0MaxPks, DevDesc_Buf );
#endif            
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN     
        s = USBHSH_GetDeviceDescr( &RootHubDev[ usb_port ].bEp0MaxPks, DevDesc_Buf );
#endif      
    }
    
    return s;
}

/*********************************************************************
 * @fn      USBH_SetUsbAddress
 *
 * @brief   Set USB device address.
 *
 * @para    index: USB host port
 *
 * @return  none
 */
uint8_t USBH_SetUsbAddress( uint8_t usb_port )
{
    uint8_t s = ERR_USB_UNSUPPORT;
    
    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        RootHubDev[ usb_port ].bAddress = (uint8_t)( DEF_USBFS_PORT_INDEX + USB_DEVICE_ADDR );
        s = USBFSH_SetUsbAddress( RootHubDev[ usb_port ].bEp0MaxPks, RootHubDev[ usb_port ].bAddress );
#endif            
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN     
        RootHubDev[ usb_port ].bAddress = (uint8_t)( DEF_USBHS_PORT_INDEX + USB_DEVICE_ADDR );
        s = USBHSH_SetUsbAddress( RootHubDev[ usb_port ].bEp0MaxPks, RootHubDev[ usb_port ].bAddress );
#endif      
    }
    
    return s;
}

/*********************************************************************
 * @fn      USBH_GetConfigDescr
 *
 * @brief   Get the configuration descriptor of the USB device. 
 *
 * @para    index: USB host port
 *
 * @return  none
 */
uint8_t USBH_GetConfigDescr( uint8_t usb_port, uint16_t *pcfg_len )
{
    uint8_t s = ERR_USB_UNSUPPORT;

    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_GetConfigDescr( RootHubDev[ usb_port ].bEp0MaxPks, Com_Buf, DEF_COM_BUF_LEN, pcfg_len );
#endif            
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN     
        s = USBHSH_GetConfigDescr( RootHubDev[ usb_port ].bEp0MaxPks, Com_Buf, DEF_COM_BUF_LEN, pcfg_len );
#endif      
    }
    
    return s;
}

/*********************************************************************
 * @fn      USBH_AnalyseType
 *
 * @brief   Simply analyze USB device type.
 *
* @para     pdev_buf: Device descriptor buffer
 *          pcfg_buf: Configuration descriptor buffer
 *          ptype: Device type.
 *
 * @return  none
 */
void USBH_AnalyseType( uint8_t *pdev_buf, uint8_t *pcfg_buf, uint8_t *ptype )
{
    uint8_t  dv_cls, if_cls;

    dv_cls = ( (PUSB_DEV_DESCR)pdev_buf )->bDeviceClass;
    if_cls = ( (PUSB_CFG_DESCR_LONG)pcfg_buf )->itf_descr.bInterfaceClass;
    if( ( dv_cls == USB_DEV_CLASS_STORAGE ) || ( if_cls == USB_DEV_CLASS_STORAGE ) )
    {
        *ptype = USB_DEV_CLASS_STORAGE;
    }
    else if( ( dv_cls == USB_DEV_CLASS_PRINTER ) || ( if_cls == USB_DEV_CLASS_PRINTER ) )
    {
        *ptype = USB_DEV_CLASS_PRINTER;
    }
    else if( ( dv_cls == USB_DEV_CLASS_HID ) || ( if_cls == USB_DEV_CLASS_HID ) )
    {
        *ptype = USB_DEV_CLASS_HID;
    }
    else if( ( dv_cls == USB_DEV_CLASS_HUB ) || ( if_cls == USB_DEV_CLASS_HUB ) )
    {
        *ptype = USB_DEV_CLASS_HUB;
    }
    else
    {
        *ptype = DEF_DEV_TYPE_UNKNOWN;
    }
}

/*********************************************************************
 * @fn      USBFSH_SetUsbConfig
 *
 * @brief   Set USB configuration.
 *
 * @para    index: USB host port
 *
 * @return  none
 */
uint8_t USBH_SetUsbConfig( uint8_t usb_port, uint8_t cfg_val )
{
    uint8_t s = ERR_USB_UNSUPPORT;
    
    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_SetUsbConfig( RootHubDev[ usb_port ].bEp0MaxPks, cfg_val );
#endif            
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN     
        s = USBHSH_SetUsbConfig( RootHubDev[ usb_port ].bEp0MaxPks, cfg_val );
#endif      
    }
    
    return s;
}

/*********************************************************************
 * @fn      USBH_GetStrDescr
 *
 * @brief   Get the string descriptor of the USB device.
 *
 * @para    index: USB host port
 *
 * @return  The result of getting the string descriptor.
 */
uint8_t USBH_GetStrDescr( uint8_t usb_port, uint8_t ep0_size, uint8_t str_num )
{
    uint8_t s = ERR_USB_UNSUPPORT;

    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_GetStrDescr( ep0_size, str_num, Com_Buf );
#endif
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN
        s = USBHSH_GetStrDescr( ep0_size, str_num, Com_Buf );
#endif
    }

    return s;
}

/*********************************************************************
 * @fn      USBH_GetHidData
 *
 * @brief
 *
 * @para    index - Corresponding host port.
 *
 * @return  none
 */
uint8_t USBH_GetHidData( uint8_t usb_port, uint8_t index, uint8_t intf_num, uint8_t endp_num, uint8_t *pbuf, uint16_t *plen )
{
    uint8_t s = ERR_USB_UNSUPPORT;

    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_GetEndpData( HostCtl[ index ].Interface[ intf_num ].InEndpAddr[ endp_num ],
                                &HostCtl[ index ].Interface[ intf_num ].InEndpTog[ endp_num ], pbuf, plen );
#endif
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN
        s = USBHSH_GetEndpData( HostCtl[ index ].Interface[ intf_num ].InEndpAddr[ endp_num ],
                                &HostCtl[ index ].Interface[ intf_num ].InEndpTog[ endp_num ], pbuf, plen );
#endif
    }

    return s;
}

/*********************************************************************
 * @fn      USBH_SendHidData
 *
 * @brief   Send data to the USB device output endpoint.
 *
 * @para    index: USB host port
 *
 * @return  The result of sending data.
 */
uint8_t USBH_SendHidData( uint8_t usb_port, uint8_t index, uint8_t intf_num, uint8_t endp_num, uint8_t *pbuf, uint16_t len )
{
    uint8_t s = ERR_USB_UNSUPPORT;

    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_SendEndpData( HostCtl[ index ].Interface[ intf_num ].OutEndpAddr[ endp_num ],
                                 &HostCtl[ index ].Interface[ intf_num ].OutEndpTog[ endp_num ], pbuf, len );
#endif
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN
        s = USBHSH_SendEndpData( HostCtl[ index ].Interface[ intf_num ].OutEndpAddr[ endp_num ],
                                 &HostCtl[ index ].Interface[ intf_num ].OutEndpTog[ endp_num ], pbuf, len );
#endif
    }

    return s;
}

/*********************************************************************
 * @fn      USBH_ClearEndpStall
 *
 * @brief
 *
 * @para    index - Corresponding host port.
 *
 * @return  none
 */
uint8_t USBH_ClearEndpStall( uint8_t usb_port, uint8_t endp_num )
{
    uint8_t s = ERR_USB_UNSUPPORT;

    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
#if DEF_USBFS_PORT_EN
        s = USBFSH_ClearEndpStall( RootHubDev[ usb_port ].bEp0MaxPks, endp_num );
#endif
    }
    else if( usb_port == DEF_USBHS_PORT_INDEX )
    {
#if DEF_USBHS_PORT_EN
        s = USBHSH_ClearEndpStall( RootHubDev[ usb_port ].bEp0MaxPks, endp_num );
#endif
    }

    return s;
}

/*********************************************************************
 * @fn      USBH_EnumRootDevice
 *
 * @brief   Generally enumerate a device connected to host port.
 *
 * @para    index: USB host port
 *
 * @return  Enumeration result
 */
uint8_t USBH_EnumRootDevice( uint8_t usb_port )
{
    uint8_t  s;
    uint8_t  enum_cnt;
    uint8_t  cfg_val;
    uint16_t i;
    uint16_t len;

    DUG_PRINTF( "Enum:\r\n" );

    enum_cnt = 0;
ENUM_START:
    /* Delay and wait for the device to stabilize */
    Delay_Ms( 100 );
    enum_cnt++;
    Delay_Ms( 8 << enum_cnt );

    /* Reset the USB device and wait for the USB device to reconnect */
    USBH_ResetRootHubPort( usb_port, 0 );
    for( i = 0, s = 0; i < DEF_RE_ATTACH_TIMEOUT; i++ )
    {
        if( USBH_EnableRootHubPort( usb_port ) == ERR_SUCCESS )
        {
            i = 0;
            s++;
            if( s > 6 )
            {
                break;
            }
        }
        Delay_Ms( 1 );
    }
    if( i )
    {
        if( usb_port == DEF_USBFS_PORT_INDEX )
        {
            USBH_PrintFsDiag( "enable_fail" );
        }
        /* Determine whether the maximum number of retries has been reached, and retry if not reached */
        if( enum_cnt <= 5 )
        {
            goto ENUM_START;
        }
        return ERR_USB_DISCON;
    }

    if( usb_port == DEF_USBFS_PORT_INDEX )
    {
        USBH_PrintFsDiag( "enabled" );
    }

    /* Get USB device device descriptor */
    DUG_PRINTF("Get DevDesc: ");
    s = USBH_GetDeviceDescr( usb_port );
    if( s == ERR_SUCCESS )
    {
        /* Print USB device device descriptor */
#if DEF_DEBUG_PRINTF
        for( i = 0; i < 18; i++ )
        {
            DUG_PRINTF( "%02x ", DevDesc_Buf[ i ] );
        }
        DUG_PRINTF("\r\n"); 
#endif
    }
    else
    {
        /* Determine whether the maximum number of retries has been reached, and retry if not reached */
        DUG_PRINTF( "Err(%02x)\r\n", s );
        if( usb_port == DEF_USBFS_PORT_INDEX )
        {
            USBH_PrintFsDiag( "getdev_err" );
        }
        if( enum_cnt <= 5 )
        {
            goto ENUM_START;
        }
        return DEF_DEV_DESCR_GETFAIL;
    }

    /* Set the USB device address */
    DUG_PRINTF("Set DevAddr: ");
    s = USBH_SetUsbAddress( usb_port );
    if( s == ERR_SUCCESS )
    {
        DUG_PRINTF( "OK\r\n" );    
    }
    else
    {
        /* Determine whether the maximum number of retries has been reached, and retry if not reached */
        DUG_PRINTF( "Err(%02x)\r\n", s );
        if( enum_cnt <= 5 )
        {
            goto ENUM_START;
        }
        return DEF_DEV_ADDR_SETFAIL;
    }
    Delay_Ms( 5 );

    /* Get the USB device configuration descriptor */
    DUG_PRINTF("Get CfgDesc: ");
    s = USBH_GetConfigDescr( usb_port, &len );
    if( s == ERR_SUCCESS )
    {
        cfg_val = ( (PUSB_CFG_DESCR)Com_Buf )->bConfigurationValue;
        
        /* Print USB device configuration descriptor  */
#if DEF_DEBUG_PRINTF
        for( i = 0; i < len; i++ )
        {
            DUG_PRINTF( "%02x ", Com_Buf[ i ] );
        }
        DUG_PRINTF("\r\n");
#endif

        /* Simply analyze USB device type  */
        USBH_AnalyseType( DevDesc_Buf, Com_Buf, &RootHubDev[ usb_port ].bType );
        DUG_PRINTF( "DevType: %02x\r\n", RootHubDev[ usb_port ].bType );
    }
    else
    {
        /* Determine whether the maximum number of retries has been reached, and retry if not reached */
        DUG_PRINTF( "Err(%02x)\r\n", s );
        if( enum_cnt <= 5 )
        {
            goto ENUM_START;
        }
        return DEF_CFG_DESCR_GETFAIL;
    }

    /* Set USB device configuration value */
    DUG_PRINTF("Set Cfg: ");
    s = USBH_SetUsbConfig( usb_port, cfg_val );
    if( s == ERR_SUCCESS )
    {
        DUG_PRINTF( "OK\r\n" );
    }
    else
    {
        /* Determine whether the maximum number of retries has been reached, and retry if not reached */
        DUG_PRINTF( "Err(%02x)\r\n", s );
        if( enum_cnt <= 5 )
        {
            goto ENUM_START;
        }
        return ERR_USB_UNSUPPORT;
    }

    return ERR_SUCCESS;
}

/*********************************************************************
 * @fn      KM_AnalyzeConfigDesc
 *
 * @brief   Analyze keyboard and mouse configuration descriptor.
 *
 * @para    index: USB host port
 *
 * @return  The result of the analysis.
 */
uint8_t KM_AnalyzeConfigDesc( uint8_t usb_port, uint8_t index )
{
    uint8_t  s = 0;
    uint16_t i;
    uint8_t  num, innum, outnum;

    num = 0;
    for( i = 0; i < ( Com_Buf[ 2 ] + ( (uint16_t)Com_Buf[ 3 ] << 8 ) ); )
    {
        if( Com_Buf[ i + 1 ] == DEF_DECR_CONFIG )
        {
            /* Save the number of interface of the USB device, only up to 4 */
            if( ( (PUSB_CFG_DESCR)( &Com_Buf[ i ] ) )->bNumInterfaces > DEF_INTERFACE_NUM_MAX )
            {
                HostCtl[ index ].InterfaceNum = DEF_INTERFACE_NUM_MAX;
            }
            else
            {
                HostCtl[ index ].InterfaceNum = ( (PUSB_CFG_DESCR)( &Com_Buf[ i ] ) )->bNumInterfaces;
            }
            i += Com_Buf[ i ];
        }
        else if( Com_Buf[ i + 1 ] == DEF_DECR_INTERFACE )
        {
            if( num == DEF_INTERFACE_NUM_MAX )
            {
                return s;
            }
            if( ( (PUSB_ITF_DESCR)( &Com_Buf[ i ] ) )->bInterfaceClass == 0x03 )
            {
                /* HID devices (such as USB keyboard and mouse) */
                if( ( (PUSB_ITF_DESCR)( &Com_Buf[ i ] ) )->bInterfaceSubClass <= 0x01 &&
                    ( (PUSB_ITF_DESCR)( &Com_Buf[ i ] ) )->bInterfaceProtocol <= 2 )
                {
                    if( ( (PUSB_ITF_DESCR)( &Com_Buf[ i ] ) )->bInterfaceProtocol == 0x01 ) // Keyboard
                    {
                        HostCtl[ index ].Interface[ num ].Type = DEC_KEY;
                        HID_SetIdle( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, num, 0, 0 );
                    }
                    else if( ( (PUSB_ITF_DESCR)( &Com_Buf[ i ] ) )->bInterfaceProtocol == 0x02 ) // Mouse
                    {
                        HostCtl[ index ].Interface[ num ].Type = DEC_MOUSE;
                        HID_SetIdle( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, num, 0, 0 );
                    }
                    s = ERR_SUCCESS;
                    i += Com_Buf[ i ];
                    innum = 0;
                    outnum = 0;
                    while( 1 )
                    {
                        if( ( Com_Buf[ i + 1 ] == DEF_DECR_INTERFACE ) || ( i >= Com_Buf[ 2 ] ) )
                        {
                            break;
                        }
                        else
                        {
                            /* Analyze each endpoint of the current interface */
                            if( Com_Buf[ i + 1 ] == DEF_DECR_ENDPOINT )
                            {
                                /* Save endpoint related information (endpoint address, attribute, max packet size, polling interval) */
                                if( ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bEndpointAddress & 0x80 )
                                {
                                    /* IN */
                                    HostCtl[ index ].Interface[ num ].InEndpAddr[ innum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bEndpointAddress & 0x0F;
                                    HostCtl[ index ].Interface[ num ].InEndpType[ innum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bmAttributes;
                                    HostCtl[ index ].Interface[ num ].InEndpSize[ innum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->wMaxPacketSizeL +
                                                                              (uint16_t)( ( ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->wMaxPacketSizeH ) << 8 );
                                    HostCtl[ index ].Interface[ num ].InEndpInterval[ innum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bInterval;
                                    HostCtl[ index ].Interface[ num ].InEndpNum++;
                                    
                                    innum++;
                                }
                                else
                                {
                                    /* OUT */
                                    HostCtl[ index ].Interface[ num ].OutEndpAddr[ outnum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bEndpointAddress & 0x0f;
                                    HostCtl[ index ].Interface[ num ].OutEndpType[ outnum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bmAttributes;
                                    HostCtl[ index ].Interface[ num ].OutEndpSize[ outnum ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->wMaxPacketSizeL +
                                                                                (uint16_t)( ( ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->wMaxPacketSizeH ) << 8 );
                                    HostCtl[ index ].Interface[ num ].OutEndpNum++;

                                    outnum++;
                                }

                                i += Com_Buf[ i ];
                            }
                            else if( Com_Buf[ i + 1 ] == DEF_DECR_HID )
                            {
                                /* Save the current interface HID report descriptor length */
                                HostCtl[ index ].Interface[ num ].HidDescLen = ( (PUSB_HID_DESCR)( &Com_Buf[ i ] ) )->wDescriptorLengthL | \
                                                                               ( (uint16_t)( ( (PUSB_HID_DESCR)( &Com_Buf[ i ] ) )->wDescriptorLengthH ) << 8 );
                                i += Com_Buf[ i ];
                            }
                            else
                            {
                                i += Com_Buf[ i ];
                            }
                        }
                    }

                    if( ( outnum == 1 ) && ( HostCtl[ index ].Interface[ num ].Type == DEC_KEY ) )
                    {
                        HostCtl[ index ].Interface[ num ].SetReport_Swi = 0xFF;
                    }
                }
                else
                {
                    HostCtl[ index ].Interface[ num ].Type = DEC_UNKNOW;
                    i += Com_Buf[ i ];
                }
            }
            else
            {
                /* USB device type unknown */
                HostCtl[ index ].Interface[ num ].Type = DEC_UNKNOW;
                i += Com_Buf[ i ];

                break;
            }
                  
            num++;
        }
        else
        {
            i += Com_Buf[ i ];
        }
    }
    
    return s;
}

/*********************************************************************
 * @fn      KM_AnalyzeHidReportDesc
 *
 * @brief   Analyze keyboard and mouse report descriptor.
 *
 * @para    index: USB host port
 *
 * @return  The result of the analysis.
 */
void KM_AnalyzeHidReportDesc( uint8_t index, uint8_t intf_num )
{
    uint8_t  id = 0x00;
    uint8_t  led = 0x00;
    uint8_t  size, type, tag;
    uint8_t  report_size;
    uint8_t  report_cnt;
    uint16_t report_bits;

    uint16_t i = 0;

    /* Usage Page(Generic Desktop), Usage(Kyeboard) */
    if( ( Com_Buf[ i + 0 ] == 0x05 ) && ( Com_Buf[ i + 1 ] == 0x01 ) &&
        ( Com_Buf[ i + 2 ] == 0x09 ) && ( Com_Buf[ i + 3 ] == 0x06 ) )
    {
        i += 4;
        report_size = 0;
        report_cnt = 0;
        report_bits = 0;

        while( i < HostCtl[ index ].Interface[ intf_num ].HidDescLen )
        {
            /* Item Size, Item Type, Item Tag */
            size = Com_Buf[ i ] & 0x03;
            type = Com_Buf[ i ] & 0x0C;
            tag = Com_Buf[ i ] & 0xF0;

            switch( type )
            {
                /* MAIN */
                case 0x00:
                    switch( tag )
                    {
                        /* Output */
                        case 0x90:
                            if( led )
                            {
                                report_bits += report_cnt * report_size;

                                /* Save report ID for output */
                                if( ( id != 0 ) && ( HostCtl[ index ].Interface[ intf_num ].IDFlag == 0 ) )
                                {
                                    HostCtl[ index ].Interface[ intf_num ].IDFlag = 1;
                                    HostCtl[ index ].Interface[ intf_num ].ReportID = id;
                                }
                            }
                            i++;
                            break;

                        default:
                            i++;
                            break;
                    }
                    break;

                /* Global */
                case 0x04:
                    switch( tag )
                    {
                        /* Report ID */
                        case 0x80:
                            i++;
                            id = Com_Buf[ i ];
                            break;

                        /* Report Count */
                        case 0x90:
                            i++;
                            report_cnt = Com_Buf[ i ];
                            break;

                        /* Report Size */
                        case 0x70:
                            i++;
                            report_size = Com_Buf[ i ];
                            break;

                        /* Usage Page */
                        case 0x00:
                            i++;
                            if( Com_Buf[ i ] == 0x08 )      // LED
                            {
                                led = 1;
                            }
                            else
                            {
                                led = 0;
                            }
                            break;

                        default:
                            i++;
                            break;
                    }
                    break;

                /* Local */
                case 0x08:
                    switch( tag )
                    {
                        /* Usage Minimum */
                        case 0x10:
                            i++;
                            if( led )
                            {
                                HostCtl[ index ].Interface[ intf_num ].LED_Usage_Min = Com_Buf[ i ];
                            }
                            break;

                        /* Usage Maximum */
                        case 0x20:
                            i++;
                            if( led )
                            {
                                HostCtl[ index ].Interface[ intf_num ].LED_Usage_Max = Com_Buf[ i ];
                            }
                            break;

                        default:
                            i++;
                            break;
                    }
                    break;

                default:
                    i++;
                    break;
            }
            i += size;
        }

        if( report_bits == 8 )
        {
            if( HostCtl[ index ].Interface[ intf_num ].SetReport_Swi == 0 )
            {
                HostCtl[ index ].Interface[ intf_num ].SetReport_Swi = 1;
            }
        }
        else
        {
            HostCtl[ index ].Interface[ intf_num ].SetReport_Swi = 0;
        }
    }
}

/*********************************************************************
 * @fn      KM_DealHidReportDesc
 *
 * @brief   Get and analyze keyboard and mouse report descriptor.
 *
 * @para    index: USB host port
 *
 * @return  The result of the acquisition and analysis.
 */
uint8_t KM_DealHidReportDesc( uint8_t usb_port, uint8_t index, uint8_t ep0_size )
{
    uint8_t  s;
    uint8_t  num, num_tmp;
    uint8_t  getrep_cnt;
#if DEF_DEBUG_PRINTF
    uint16_t i;
#endif

    getrep_cnt = 0;
    num_tmp = HostCtl[ index ].InterfaceNum;
    while( num_tmp )
    {
        num = HostCtl[ index ].InterfaceNum - num_tmp;
        if( HostCtl[ index ].Interface[ num ].HidDescLen )
        {
GETREP_START:
            getrep_cnt++;
            
            /* Get HID report descriptor */
            DUG_PRINTF("Get Interface%x RepDesc: ", num );
            s = HID_GetHidDesr( usb_port, ep0_size, num, Com_Buf, &HostCtl[ index ].Interface[ num ].HidDescLen );
            if( s == ERR_SUCCESS )
            {
                /* Print HID report descriptor */
#if DEF_DEBUG_PRINTF
                for( i = 0; i < HostCtl[ index ].Interface[ num ].HidDescLen; i++ )
                {
                    DUG_PRINTF( "%02x " , Com_Buf[ i ]);
                }
                DUG_PRINTF("\r\n");
#endif

                /* Analyze Report Descriptor */
                KM_AnalyzeHidReportDesc( index, num );
                if( HostCtl[ index ].Interface[ num ].Type == DEC_MOUSE )
                {
                    USBH_MouseBridge_AnalyzeMouseReportDesc( index, num, Com_Buf, HostCtl[ index ].Interface[ num ].HidDescLen );
                }

                num_tmp--;
            }
            else
            {
                DUG_PRINTF( "Err(%02x)\r\n", s );
                if( getrep_cnt <= 5 )
                {
                    goto GETREP_START;
                }

                return DEF_REP_DESCR_GETFAIL;
            }
        }
        else
        {
            num_tmp--;
        }
    }

    return s;
}

/*********************************************************************
 * @fn      USBH_EnumHidDevice
 *
 * @brief   Enumerate HID device.
 *
 * @para    index: USB host port
 *
 * @return  The result of the enumeration.
 */
uint8_t USBH_EnumHidDevice( uint8_t usb_port, uint8_t index, uint8_t ep0_size )
{
    uint8_t  s;
    uint8_t  intf_num;
#if DEF_DEBUG_PRINTF
    uint8_t  i;
#endif

    DUG_PRINTF( "Enum Hid:\r\n" );
    
    /* Analyze HID class device configuration descriptor and save relevant parameters */
    DUG_PRINTF("Analyze CfgDesc: ");
    s = KM_AnalyzeConfigDesc( usb_port, index );
    if( s == ERR_SUCCESS )
    {
        DUG_PRINTF( "OK\r\n" );
    }
    else
    {
        DUG_PRINTF( "Err(%02x)\r\n", s );
        return s;
    }

    /* Get the string descriptor contained in the configuration descriptor if it exists */
    if( Com_Buf[ 6 ] )
    {
        DUG_PRINTF("Get StringDesc4: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, Com_Buf[ 6 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print the string descriptor contained in the configuration descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get HID report descriptor */
    s = KM_DealHidReportDesc( usb_port, index, ep0_size );
    
    /* Get USB vendor string descriptor  */
    if( DevDesc_Buf[ 14 ] )
    {
        DUG_PRINTF("Get StringDesc1: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, DevDesc_Buf[ 14 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print USB vendor string descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ]);
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get USB product string descriptor */
    if( DevDesc_Buf[ 15 ] )
    {
        DUG_PRINTF("Get StringDesc2: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, DevDesc_Buf[ 15 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print USB product string descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get USB serial number string descriptor */
    if( DevDesc_Buf[ 16 ] )
    {
        DUG_PRINTF("Get StringDesc3: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, DevDesc_Buf[ 16 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print USB serial number string descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get USB serial number string descriptor */
    for( intf_num = 0; intf_num < HostCtl[ index ].InterfaceNum; intf_num++ )
    {
        if( HostCtl[ index ].Interface[ intf_num ].Type == DEC_KEY )
        {
            HostCtl[ index ].Interface[ intf_num ].SetReport_Value = 0x00;
            KB_SetReport( usb_port, index, ep0_size, intf_num );
        }
    }

    return ERR_SUCCESS;
}

/*********************************************************************
 * @fn      HUB_Analyse_ConfigDesc
 *
 * @brief   Analyze HUB configuration descriptor.
 *
 * @para
 *
 * @return  none
 */
uint8_t HUB_AnalyzeConfigDesc( uint8_t index )
{
    uint8_t  s = ERR_SUCCESS;
    uint16_t i;

    for( i = 0; i < ( Com_Buf[ 2 ] + ( (uint16_t)Com_Buf[ 3 ] << 8 ) ); )
    {
        if( Com_Buf[ i + 1 ] == DEF_DECR_CONFIG )
        {
            /* Save the number of interface of the USB device, only up to 4 */
            if( ( (PUSB_CFG_DESCR)( &Com_Buf[ i ] ) )->bNumInterfaces > 1 )
            {
                HostCtl[ index ].InterfaceNum = 1;
            }
            else
            {
                HostCtl[ index ].InterfaceNum = ( (PUSB_CFG_DESCR)( &Com_Buf[ i ] ) )->bNumInterfaces;
            }
            i += Com_Buf[ i ];
        }
        else if( Com_Buf[ i + 1 ] == DEF_DECR_INTERFACE )
        {
            if( ( (PUSB_ITF_DESCR)( &Com_Buf[ i ] ) )->bInterfaceClass == 0x09 )
            {
                i += Com_Buf[ i ];
                while( 1 )
                {
                    if( ( Com_Buf[ i + 1 ] == DEF_DECR_INTERFACE ) || ( i >= ( Com_Buf[ 2 ] + ( (uint16_t)Com_Buf[ 3 ] << 8 ) ) ) )
                    {
                        break;
                    }
                    else
                    {
                        /* Analyze each endpoint of the current interface */
                        if( Com_Buf[ i + 1 ] == DEF_DECR_ENDPOINT )
                        {
                            /* Save endpoint related information (endpoint address, attribute, max packet size, polling interval) */
                            if( ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bEndpointAddress & 0x80 )
                            {
                                /* IN */
                                HostCtl[ index ].Interface[ 0 ].InEndpAddr[ 0 ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bEndpointAddress & 0x0F;
                                HostCtl[ index ].Interface[ 0 ].InEndpType[ 0 ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bmAttributes;
                                HostCtl[ index ].Interface[ 0 ].InEndpSize[ 0 ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->wMaxPacketSizeL + \
                                                                              (uint16_t)( ( ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->wMaxPacketSizeH ) << 8 );
                                HostCtl[ index ].Interface[ 0 ].InEndpInterval[ 0 ] = ( (PUSB_ENDP_DESCR)( &Com_Buf[ i ] ) )->bInterval;
                                HostCtl[ index ].Interface[ 0 ].InEndpNum++;
                            }

                            i += Com_Buf[ i ];
                        }
                        else
                        {
                            i += Com_Buf[ i ];
                        }
                    }
                }
            }
            else
            {
                /* USB device type unknown */
                i += Com_Buf[ i ];
            }
        }
        else
        {
            i += Com_Buf[ i ];
        }
    }
    return s;
}

/*********************************************************************
 * @fn      USBH_EnumHubDevice
 *
 * @brief   Enumerate HUB device.
 *
 * @para    index: USB host port
 *
 * @return  The result of the enumeration.
 */
uint8_t USBH_EnumHubDevice( uint8_t usb_port, uint8_t ep0_size )
{
    uint8_t  s, retry;
    uint16_t len;
    uint16_t  i;

    DUG_PRINTF( "Enum Hub:\r\n" );

    /* Analyze HID class device configuration descriptor and save relevant parameters */
    DUG_PRINTF("Analyze CfgDesc: ");
    s = HUB_AnalyzeConfigDesc( RootHubDev[ usb_port ].DeviceIndex );
    if( s == ERR_SUCCESS )
    {
        DUG_PRINTF( "OK\r\n" );
    }
    else
    {
        DUG_PRINTF( "Err(%02x)\r\n", s );
        return s;
    }

    /* Get the string descriptor contained in the configuration descriptor if it exists */
    if( Com_Buf[ 6 ] )
    {
        DUG_PRINTF("Get StringDesc4: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, Com_Buf[ 6 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print the string descriptor contained in the configuration descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get USB vendor string descriptor  */
    if( DevDesc_Buf[ 14 ] )
    {
        DUG_PRINTF("Get StringDesc1: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, DevDesc_Buf[ 14 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print USB vendor string descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ]);
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get USB product string descriptor */
    if( DevDesc_Buf[ 15 ] )
    {
        DUG_PRINTF("Get StringDesc2: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, DevDesc_Buf[ 15 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print USB product string descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get USB serial number string descriptor */
    if( DevDesc_Buf[ 16 ] )
    {
        DUG_PRINTF("Get StringDesc3: ");
        s = USBH_GetStrDescr( usb_port, ep0_size, DevDesc_Buf[ 16 ] );
        if( s == ERR_SUCCESS )
        {
            /* Print USB serial number string descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < Com_Buf[ 0 ]; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
        }
    }

    /* Get hub descriptor */
    DUG_PRINTF("Get Hub Desc: ");
    for( retry = 0; retry < 5; retry++ )
    {
        s = HUB_GetClassDevDescr( usb_port, ep0_size, Com_Buf, &len );
        if( s == ERR_SUCCESS )
        {
            /* Print USB device device descriptor */
#if DEF_DEBUG_PRINTF
            for( i = 0; i < len; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF("\r\n");
#endif

            RootHubDev[ usb_port ].bPortNum = ( (PUSB_HUB_DESCR)Com_Buf)->bNbrPorts;
            if( RootHubDev[ usb_port ].bPortNum > DEF_NEXT_HUB_PORT_NUM_MAX )
            {
                RootHubDev[ usb_port ].bPortNum = DEF_NEXT_HUB_PORT_NUM_MAX;
            }
            DUG_PRINTF( "RootHubDev[ %02x ].bPortNum: %02x\r\n", usb_port, RootHubDev[ usb_port ].bPortNum );
            break;
        }
        else
        {
            /* Determine whether the maximum number of retries has been reached, and retry if not reached */
            DUG_PRINTF( "Err(%02x)\r\n", s );

            if( retry == 4 )
            {
                return ERR_USB_UNKNOWN;
            }
        }
    }

    /* Set the HUB port to power on */
    for( retry = 0, i = 1; i <= RootHubDev[ usb_port ].bPortNum; i++ )
    {
        s = HUB_SetPortFeature( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, i, HUB_PORT_POWER );
        if( s == ERR_SUCCESS )
        {
            continue;
        }
        else
        {
            Delay_Ms( 5 );

            i--;
            retry++;
            if( retry >= 5 )
            {
                return ERR_USB_UNKNOWN;
            }
        }
    }
    
    return ERR_SUCCESS;
}

/*********************************************************************
 * @fn      HUB_Port_PreEnum1
 *
 * @brief
 *
 * @para
 *
 * @return  none
 */
uint8_t HUB_Port_PreEnum1( uint8_t usb_port, uint8_t hub_port, uint8_t *pbuf )
{
    uint8_t  s;
    uint8_t  buf[ 4 ];
    uint8_t  retry;

    if( ( *pbuf ) & ( 1 << hub_port ) )
    {
        s = HUB_GetPortStatus( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, &buf[ 0 ] );
        if( s != ERR_SUCCESS )
        {
            DUG_PRINTF( "HUB_PE1_ERR1:%x\r\n", s );
            return s;
        }
        else
        {
            if( buf[ 2 ] & 0x01 )
            {
                s = HUB_ClearPortFeature( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, HUB_C_PORT_CONNECTION );
                if( s != ERR_SUCCESS )
                {
                    DUG_PRINTF( "HUB_PE1_ERR2:%x\r\n", s );
                    return s;
                }

                retry = 0;
                do
                {
                    s = HUB_GetPortStatus( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, &buf[ 0 ] );
                    if( s != ERR_SUCCESS )
                    {
                        DUG_PRINTF( "HUB_PE1_ERR3:%x\r\n", s );
                        return s;
                    }
                    retry++;
                }while( ( buf[ 2 ] & 0x01 ) && ( retry < 10 ) );

                if( retry != 10 )
                {
                    if( !( buf[ 0 ] & 0x01 ) )
                    {
                        DUG_PRINTF( "Hub Port%x Out\r\n", hub_port );
                        return ERR_USB_DISCON;
                    }
                }
            }
        }
    }

    return ERR_USB_UNKNOWN;
}

/*********************************************************************
 * @fn      HUB_Port_PreEnum2
 *
 * @brief
 *
 * @para
 *
 * @return  none
 */
uint8_t HUB_Port_PreEnum2( uint8_t usb_port, uint8_t hub_port, uint8_t *pbuf )
{
    uint8_t  s;
    uint8_t  buf[ 4 ];
    uint8_t  retry = 0;

    if( ( *pbuf ) & ( 1 << hub_port ) )
    {
        s = HUB_SetPortFeature( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, HUB_PORT_RESET );
        if( s != ERR_SUCCESS )
        {
            DUG_PRINTF( "HUB_PE2_ERR1:%x\r\n", s );
            return s;
        }

        do
        {
            s = HUB_GetPortStatus( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, &buf[ 0 ] );
            if( s != ERR_SUCCESS )
            {
                DUG_PRINTF( "HUB_PE2_ERR2:%x\r\n", s );
                return s;
            }
            retry++;
            Delay_Ms(1);
        }while( ( !( buf[ 2 ] & 0x10 ) ) && ( retry <= 100 ) );

        if( retry != 100 )
        {
            retry = 0;
            s = HUB_ClearPortFeature( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, HUB_C_PORT_RESET  );

            do
            {
                s = HUB_GetPortStatus( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, &buf[ 0 ] );
                if( s != ERR_SUCCESS )
                {
                    DUG_PRINTF( "HUB_PE2_ERR3:%x\r\n", s );
                    return s;
                }
                retry++;
            }while( ( buf[ 2 ] & 0x10 ) && ( retry <= 10 ) ); // ����˿ڸ�λ���

            if( retry != 10 )
            {
                if( buf[ 0 ] & 0x01 )
                {
                    DUG_PRINTF( "Hub Port%x In\r\n", hub_port );
                    return ERR_USB_CONNECT;
                }
            }
        }
    }

    return ERR_USB_UNKNOWN;
}

/*********************************************************************
 * @fn      HUB_CheckPortSpeed
 *
 * @brief
 *
 * @para
 *
 * @return  none
 */
uint8_t HUB_CheckPortSpeed( uint8_t usb_port, uint8_t hub_port, uint8_t *pbuf )
{
    uint8_t  s;

    s = HUB_GetPortStatus( usb_port, RootHubDev[ usb_port ].bEp0MaxPks, hub_port, pbuf );
    if( s )
    {
        return s;
    }

    if( pbuf[ 1 ] & 0x02 )
    {
        return USB_LOW_SPEED;
    }
    else
    {
        if( pbuf[ 1 ] & 0x04 )
        {
            return USB_HIGH_SPEED;
        }
        else
        {
            return USB_FULL_SPEED;
        }
    }
}

/*********************************************************************
 * @fn      USBH_EnumHubPortDevice
 *
 * @brief
 *
 * @para
 *
 * @return  none
 */
uint8_t USBH_EnumHubPortDevice( uint8_t usb_port, uint8_t hub_port, uint8_t *paddr, uint8_t *ptype )
{
    uint8_t  s;
    uint8_t  enum_cnt;
    uint16_t len;
    uint8_t  cfg_val;
#if DEF_DEBUG_PRINTF
    uint16_t i;
#endif

    /* Get USB device descriptor */
    DUG_PRINTF("(S1)Get DevDesc: \r\n");
    enum_cnt = 0;
    do
    {
        enum_cnt++;
        s = USBFSH_GetDeviceDescr( &RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks, DevDesc_Buf );
        if( s == ERR_SUCCESS )
        {
#if DEF_DEBUG_PRINTF
            for( i = 0; i < 18; i++ )
            {
                DUG_PRINTF( "%02x ", DevDesc_Buf[ i ] );
            }
            DUG_PRINTF( "\r\n" );
#endif
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
            if( enum_cnt >= 10 )
            {
                return DEF_DEV_DESCR_GETFAIL;
            }
        }
    }while( ( s != ERR_SUCCESS ) && ( enum_cnt < 10 ) );

    /* Set the USB device address */
    DUG_PRINTF( "Set DevAddr: \r\n" );
    enum_cnt = 0;
    do
    {
        enum_cnt++;
        s = USBFSH_SetUsbAddress( RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks, \
                                  RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex + USB_DEVICE_ADDR );
        if( s == ERR_SUCCESS )
        {
            /* Save address */
            *paddr = RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex + USB_DEVICE_ADDR;
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
            if( enum_cnt >= 10 )
            {
                return DEF_DEV_ADDR_SETFAIL;
            }
        }
    }while( ( s != ERR_SUCCESS ) && ( enum_cnt < 10 ) );
    Delay_Ms( 5 );

    /* Get USB configuration descriptor */
    DUG_PRINTF( "Get DevCfgDesc: \r\n" );
    enum_cnt = 0;
    do
    {
        enum_cnt++;
        s = USBFSH_GetConfigDescr( RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks, Com_Buf, DEF_COM_BUF_LEN, &len );
        if( s == ERR_SUCCESS )
        {
#if DEF_DEBUG_PRINTF
            for( i = 0; i < len; i++ )
            {
                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
            }
            DUG_PRINTF( "\r\n" );
#endif

            /* Save configuration value */
            cfg_val = ( (PUSB_CFG_DESCR)Com_Buf )->bConfigurationValue;

            /* Analyze USB device type */
            USBH_AnalyseType( DevDesc_Buf, Com_Buf, ptype );
            DUG_PRINTF( "DevType: %02x\r\n", *ptype );
        }
        else
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
            if( enum_cnt >= 10 )
            {
                return DEF_DEV_DESCR_GETFAIL;
            }
        }
    }while( ( s != ERR_SUCCESS ) && ( enum_cnt < 10 ) );

    /* Set USB device configuration value */
    DUG_PRINTF( "Set CfgValue: \r\n" );
    enum_cnt = 0;
    do
    {
        enum_cnt++;
        s = USBFSH_SetUsbConfig( RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks, cfg_val );
        if( s != ERR_SUCCESS )
        {
            DUG_PRINTF( "Err(%02x)\r\n", s );
            if( enum_cnt >= 10 )
            {
                return DEF_CFG_VALUE_SETFAIL;
            }
        }
    }while( ( s != ERR_SUCCESS ) && ( enum_cnt < 10 ) );

    return( ERR_SUCCESS );
}

/*********************************************************************
 * @fn      KB_AnalyzeKeyValue
 *
 * @brief   Handle keyboard lighting.
 *
 * @para    index: USB host port
 *          intfnum: Interface number.
 *          pbuf: Data buffer.
 *          len: Data length.
 *
 * @return  The result of the analysis.
 */
void KB_AnalyzeKeyValue( uint8_t index, uint8_t intf_num, uint8_t *pbuf, uint16_t len )
{
    uint8_t  i;
    uint8_t  value;
    uint8_t  bit_pos = 0x00;

    value = HostCtl[ index ].Interface[ intf_num ].SetReport_Value;

    for( i = HostCtl[ index ].Interface[ intf_num ].LED_Usage_Min; i <= HostCtl[ index ].Interface[ intf_num ].LED_Usage_Max; i++ )
    {
        if( i == 0x01 )
        {
            if( memchr( pbuf, DEF_KEY_NUM, len ) )
            {
                HostCtl[ index ].Interface[ intf_num ].SetReport_Value ^= ( 1 << bit_pos );
            }
        }
        else if( i == 0x02 )
        {
            if( memchr( pbuf, DEF_KEY_CAPS, len ) )
            {
                HostCtl[ index ].Interface[ intf_num ].SetReport_Value ^= ( 1 << bit_pos );
            }
        }
        else if( i == 0x03 )
        {
            if( memchr( pbuf, DEF_KEY_SCROLL, len ) )
            {
                HostCtl[ index ].Interface[ intf_num ].SetReport_Value ^= ( 1 << bit_pos );
            }
        }

        bit_pos++;
    }

    if( value != HostCtl[ index ].Interface[ intf_num ].SetReport_Value )
    {
        HostCtl[ index ].Interface[ intf_num ].SetReport_Flag = 1;
    }
    else
    {
        HostCtl[ index ].Interface[ intf_num ].SetReport_Flag = 0;
    }
}

/*********************************************************************
 * @fn      KB_SetReport
 *
 * @brief   Handle keyboard lighting.
 *
 * @para    index: USB device number.
 *          intf_num: Interface number.
 *
 * @return  The result of the handling keyboard lighting.
 */
uint8_t KB_SetReport( uint8_t usb_port, uint8_t index, uint8_t ep0_size, uint8_t intf_num )
{
    uint8_t  dat[ 2 ];
    uint16_t len;

    if( HostCtl[ index ].Interface[ intf_num ].IDFlag )
    {
        dat[ 0 ] = HostCtl[ index ].Interface[ intf_num ].ReportID;
        dat[ 1 ] = HostCtl[ index ].Interface[ intf_num ].SetReport_Value;
        len = 2;
    }
    else
    {
        dat[ 0 ] = HostCtl[ index ].Interface[ intf_num ].SetReport_Value;
        len = 1;
    }

    if( HostCtl[ index ].Interface[ intf_num ].SetReport_Swi == 1 ) // Perform lighting operation through endpoint0
    {
        /* Send set report command */
        return HID_SetReport( usb_port, ep0_size, intf_num, dat, &len );
    }
    else if( HostCtl[ index ].Interface[ intf_num ].SetReport_Swi == 0xFF )  // Perform lighting operation through other endpoint
    {
        return USBH_SendHidData( usb_port, index, intf_num, 0, dat, len );
    }

    return ERR_SUCCESS;
}

/*********************************************************************
 * @fn      USBH_MainDeal
 *
 * @brief   Provide a simple enumeration process for USB devices and
 *          obtain keyboard and mouse data at regular intervals.
 *
 * @return  none
 */
void USBH_MainDeal( void )
{
    uint8_t  s;
    uint8_t  usb_port;
#if DEF_USBFS_PORT_EN
    uint8_t  hub_port;
    uint8_t  hub_dat;
#endif
    uint8_t  index;
    uint8_t  intf_num, in_num;
    uint16_t len;
#if DEF_DEBUG_PRINTF
    uint16_t i;
#endif
    
    for( usb_port = 0; usb_port < DEF_TOTAL_ROOT_HUB; usb_port++ )
    {
        s = USBH_CheckRootHubPortStatus( usb_port ); // Check USB device connection or disconnection
        if( s == ROOT_DEV_CONNECTED )
        {
            DUG_PRINTF( "USB Port%x Dev In.\r\n", usb_port );
            
            /* Set root device state parameters */
            RootHubDev[ usb_port ].bStatus = ROOT_DEV_CONNECTED;
            RootHubDev[ usb_port ].DeviceIndex = usb_port * DEF_ONE_USB_SUP_DEV_TOTAL;

            s = USBH_EnumRootDevice( usb_port ); // Simply enumerate root device
            if( s == ERR_SUCCESS )
            {
                if( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HID ) // Further enumerate it if this device is a HID device
                {
                    DUG_PRINTF("Root Device Is HID. ");

                    s = USBH_EnumHidDevice( usb_port, RootHubDev[ usb_port ].DeviceIndex, RootHubDev[ usb_port ].bEp0MaxPks );
                    DUG_PRINTF( "Further Enum Result: " );
                    if( s == ERR_SUCCESS )
                    {
                        DUG_PRINTF( "OK\r\n" );
                        
                        /* Set the connection status of the device  */
                        RootHubDev[ usb_port ].bStatus = ROOT_DEV_SUCCESS;
                    }
                    else if( s != ERR_USB_DISCON )
                    {
                        DUG_PRINTF( "Err(%02x)\r\n", s );
                        
                        RootHubDev[ usb_port ].bStatus = ROOT_DEV_FAILED;
                    }
                }
                else if( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HUB )
                {
                    DUG_PRINTF("Root Device Is HUB. ");

                    s = USBH_EnumHubDevice( usb_port, RootHubDev[ usb_port ].bEp0MaxPks );
                    DUG_PRINTF( "Further Enum Result: " );
                    if( s == ERR_SUCCESS )
                    {
                        DUG_PRINTF( "OK\r\n" );

                        /* Set the connection status of the device  */
                        RootHubDev[ usb_port ].bStatus = ROOT_DEV_SUCCESS;
                    }
                    else if( s != ERR_USB_DISCON )
                    {
                        DUG_PRINTF( "Err(%02x)\r\n", s );

                        RootHubDev[ usb_port ].bStatus = ROOT_DEV_FAILED;
                    }
                }
                else // Detect that this device is a Non-HID device
                {
                    DUG_PRINTF( "Root Device Is " );
                    switch( RootHubDev[ usb_port ].bType )
                    {
                        case USB_DEV_CLASS_STORAGE:
                            DUG_PRINTF("Storage. ");
                            break;
                        case USB_DEV_CLASS_PRINTER:
                            DUG_PRINTF("Printer. ");
                            break;
                        case DEF_DEV_TYPE_UNKNOWN:
                            DUG_PRINTF("Unknown. ");
                            break;
                    }
                    DUG_PRINTF( "End Enum.\r\n" );
                    
                    RootHubDev[ usb_port ].bStatus = ROOT_DEV_SUCCESS;
                }
            }
            else if( s != ERR_USB_DISCON )
            {
                /* Enumeration failed */
                DUG_PRINTF( "Enum Fail with Error Code:%x\r\n",s );
                RootHubDev[ usb_port ].bStatus = ROOT_DEV_FAILED;
            }
        }
        else if( s == ROOT_DEV_DISCONNECT )
        {
            DUG_PRINTF( "USB Port%x Dev Out.\r\n", usb_port );
            
            /* Clear parameters */
            index = RootHubDev[ usb_port ].DeviceIndex;
            memset( &RootHubDev[ usb_port ].bStatus, 0, sizeof( ROOT_HUB_DEVICE ) );
            memset( &HostCtl[ index ].InterfaceNum, 0, sizeof( HOST_CTL ) );
        }
    }

    /* Get the data of the HID device connected to the USB host port */
    for( usb_port = 0; usb_port < DEF_TOTAL_ROOT_HUB; usb_port++ )
    {
        if( RootHubDev[ usb_port ].bStatus >= ROOT_DEV_SUCCESS )
        {
            index = RootHubDev[ usb_port ].DeviceIndex;
            if( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HID )
            {
                for( intf_num = 0; intf_num < HostCtl[ index ].InterfaceNum; intf_num++ )
                {
                    for( in_num = 0; in_num < HostCtl[ index ].Interface[ intf_num ].InEndpNum; in_num++ )
                    {                   
                        /* Get endpoint data based on the interval time of the device */
                        if( HostCtl[ index ].Interface[ intf_num ].InEndpTimeCount[ in_num ] >= HostCtl[ index ].Interface[ intf_num ].InEndpInterval[ in_num ] )
                        {
                            HostCtl[ index ].Interface[ intf_num ].InEndpTimeCount[ in_num ] %= HostCtl[ index ].Interface[ intf_num ].InEndpInterval[ in_num ];
               
                            /* Get endpoint data */
                            s = USBH_GetHidData( usb_port, index, intf_num, in_num, Com_Buf, &len );
                            if( s == ERR_SUCCESS )
                            {
#if DEF_DEBUG_PRINTF
                                for( i = 0; i < len; i++ )
                                {
                                    DUG_PRINTF( "%02x ", Com_Buf[ i ] );
                                }
                                DUG_PRINTF( "\r\n" );
#endif
                                
                                /* Handle keyboard lighting */
                                if( HostCtl[ index ].Interface[ intf_num ].Type == DEC_KEY )
                                {
                                    KB_AnalyzeKeyValue( index, intf_num, Com_Buf, len );

                                    if( HostCtl[ index ].Interface[ intf_num ].SetReport_Flag )
                                    {
                                        KB_SetReport( usb_port, index, RootHubDev[ usb_port ].bEp0MaxPks, intf_num );
                                    }
                                }
                                else if( HostCtl[ index ].Interface[ intf_num ].Type == DEC_MOUSE )
                                {
                                    USBH_MouseBridge_StoreReport( index, intf_num, Com_Buf, len );
                                }
                            }
                            else if( s == ERR_USB_DISCON )
                            {
                                break;
                            }
                            else if( s == ( USB_PID_STALL | ERR_USB_TRANSFER ) )
                            {
                                /* USB device abnormal event */
                                DUG_PRINTF("Abnormal\r\n");
                                
                                /* Clear endpoint */
                                USBH_ClearEndpStall( usb_port, HostCtl[ index ].Interface[ intf_num ].InEndpAddr[ in_num ] | 0x80 );
                                HostCtl[ index ].Interface[ intf_num ].InEndpTog[ in_num ] = 0x00;
                                
                                /* Judge the number of error */
                                HostCtl[ index ].ErrorCount++;
                                if( HostCtl[ index ].ErrorCount >= 10 )
                                {
                                    /* Re-enumerate the device and clear the endpoint again */
                                    memset( &RootHubDev[ usb_port ].bStatus, 0, sizeof( ROOT_HUB_DEVICE ) );
                                    s = USBH_EnumRootDevice( usb_port );
                                    if( s == ERR_SUCCESS )
                                    {
                                        USBH_ClearEndpStall( usb_port, HostCtl[ index ].Interface[ intf_num ].InEndpAddr[ in_num ] | 0x80 );
                                        HostCtl[ index ].ErrorCount = 0x00;
                                        
                                        RootHubDev[ usb_port ].bStatus = ROOT_DEV_CONNECTED; 
                                        RootHubDev[ usb_port ].DeviceIndex = usb_port * DEF_ONE_USB_SUP_DEV_TOTAL;
                                        
                                        memset( &HostCtl[ index ].InterfaceNum, 0, sizeof( HOST_CTL ) );
                                        s = USBH_EnumHidDevice( usb_port, index, RootHubDev[ usb_port ].bEp0MaxPks );
                                        if( s == ERR_SUCCESS )
                                        {
                                            RootHubDev[ usb_port ].bStatus = ROOT_DEV_SUCCESS; 
                                        }
                                        else if( s != ERR_USB_DISCON )
                                        {
                                            RootHubDev[ usb_port ].bStatus = ROOT_DEV_FAILED; 
                                        }
                                    }
                                    else if( s != ERR_USB_DISCON )
                                    {
                                        RootHubDev[ usb_port ].bStatus = ROOT_DEV_FAILED;
                                    }
                                }
                            }
                        }
                    }

                    if( s == ERR_USB_DISCON )
                    {
                        break;
                    }
                }
            }
#if DEF_USBFS_PORT_EN
            else if( ( usb_port == DEF_USBFS_PORT_INDEX ) && ( RootHubDev[ usb_port ].bType == USB_DEV_CLASS_HUB ) )
            {
                /* Query port status change */
                if( HostCtl[ index ].Interface[ 0 ].InEndpTimeCount[ 0 ] >= HostCtl[ index ].Interface[ 0 ].InEndpInterval[ 0 ] )
                {
                    HostCtl[ index ].Interface[ 0 ].InEndpTimeCount[ 0 ] %= HostCtl[ index ].Interface[ 0 ].InEndpInterval[ 0 ];

                    /* Select HUB port */
                    USBFSH_SetSelfAddr( RootHubDev[ usb_port ].bAddress );
                    USBFSH_SetSelfSpeed( RootHubDev[ usb_port ].bSpeed );

                    /* Get HUB interrupt endpoint data */
                    s = USBFSH_GetEndpData( HostCtl[ index ].Interface[ 0 ].InEndpAddr[ 0 ], &HostCtl[ index ].Interface[ 0 ].InEndpTog[ 0 ], Com_Buf, &len );
                    if( s == ERR_SUCCESS )
                    {
                        hub_dat = Com_Buf[ 0 ];
                        DUG_PRINTF( "Hub Int Data:%02x\r\n", hub_dat );

                        for( hub_port = 0; hub_port < RootHubDev[ usb_port ].bPortNum; hub_port++ )
                        {
                            /* HUB Port PreEnumate Step 1: C_PORT_CONNECTION */
                            s = HUB_Port_PreEnum1( usb_port, ( hub_port + 1 ), &hub_dat );
                            if( s == ERR_USB_DISCON )
                            {
                                hub_dat &= ~( 1 << ( hub_port + 1 ) );

                                /* Clear parameters */
                                memset( &HostCtl[ RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex ], 0, sizeof( HOST_CTL ) );
                                memset( &RootHubDev[ usb_port ].Device[ hub_port ].bStatus, 0, sizeof( HUB_DEVICE ) );
                                continue;
                            }

                            /* HUB Port PreEnumate Step 2: Set/Clear PORT_RESET */
                            Delay_Ms( 100 );
                            s = HUB_Port_PreEnum2( usb_port, ( hub_port + 1 ), &hub_dat );
                            if( s == ERR_USB_CONNECT )
                            {
                                /* Set parameters */
                                RootHubDev[ usb_port ].Device[ hub_port ].bStatus = ROOT_DEV_CONNECTED;
                                RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks = DEFAULT_ENDP0_SIZE;
                                RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex = usb_port * DEF_ONE_USB_SUP_DEV_TOTAL + hub_port + 1;
                            }
                            else
                            {
                                hub_dat &= ~( 1 << ( hub_port + 1 ) );
                            }

                            /* Enumerate HUB Device */
                            if( RootHubDev[ usb_port ].Device[ hub_port ].bStatus == ROOT_DEV_CONNECTED )
                            {
                                /* Check device speed */
                                RootHubDev[ usb_port ].Device[ hub_port ].bSpeed = HUB_CheckPortSpeed( usb_port, ( hub_port + 1 ), Com_Buf );
                                DUG_PRINTF( "Dev Speed:%x\r\n", RootHubDev[ usb_port ].Device[ hub_port ].bSpeed );

                                /* Select the specified port */
                                USBFSH_SetSelfAddr( RootHubDev[ usb_port ].Device[ hub_port ].bAddress );
                                USBFSH_SetSelfSpeed( RootHubDev[ usb_port ].Device[ hub_port ].bSpeed );
                                if( RootHubDev[ usb_port ].bSpeed != USB_LOW_SPEED )
                                {
                                    USBFSH->HOST_CTRL &= ~USBFS_UH_LOW_SPEED;
                                }

                                /* Enumerate the USB device of the current HUB port */
                                DUG_PRINTF("Enum_HubDevice\r\n");
                                s = USBH_EnumHubPortDevice( usb_port, hub_port, &RootHubDev[ usb_port ].Device[ hub_port ].bAddress, \
                                                            &RootHubDev[ usb_port ].Device[ hub_port ].bType );
                                if( s == ERR_SUCCESS )
                                {
                                    if( RootHubDev[ usb_port ].Device[ hub_port ].bType == USB_DEV_CLASS_HID )
                                    {
                                        DUG_PRINTF( "HUB port%x device is HID! Further Enum:\r\n", hub_port );

                                        /* Perform HID class enumeration on the current device */
                                        s = USBH_EnumHidDevice( usb_port, RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex, \
                                                                RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks );
                                        if( s == ERR_SUCCESS )
                                        {
                                            RootHubDev[ usb_port ].Device[ hub_port ].bStatus = ROOT_DEV_SUCCESS;
                                            DUG_PRINTF( "OK!\r\n" );
                                        }
                                    }
                                    else // Detect that this device is a Non-HID device
                                    {
                                        DUG_PRINTF( "HUB port%x device is ", hub_port );
                                        switch( RootHubDev[ usb_port ].Device[ hub_port ].bType )
                                        {
                                            case USB_DEV_CLASS_STORAGE:
                                                DUG_PRINTF("storage!\r\n");
                                                break;
                                            case USB_DEV_CLASS_PRINTER:
                                                DUG_PRINTF("printer!\r\n");
                                                break;
                                            case USB_DEV_CLASS_HUB:
                                                DUG_PRINTF("printer!\r\n");
                                                break;
                                            case DEF_DEV_TYPE_UNKNOWN:
                                                DUG_PRINTF("unknown!\r\n");
                                                break;
                                        }
                                        RootHubDev[ usb_port ].Device[ hub_port ].bStatus = ROOT_DEV_SUCCESS;
                                    }
                                }
                                else
                                {
                                    RootHubDev[ usb_port ].Device[ hub_port ].bStatus = ROOT_DEV_FAILED;
                                    DUG_PRINTF( "HUB Port%x Enum Err!\r\n", hub_port );
                                }
                            }
                        }
                    }
                }

                /* Get HUB port HID device data */
                for( hub_port = 0; hub_port < RootHubDev[ usb_port ].bPortNum; hub_port++ )
                {
                    if( RootHubDev[ usb_port ].Device[ hub_port ].bStatus == ROOT_DEV_SUCCESS )
                    {
                        index = RootHubDev[ usb_port ].Device[ hub_port ].DeviceIndex;

                        if( RootHubDev[ usb_port ].Device[ hub_port ].bType == USB_DEV_CLASS_HID )
                        {
                            for( intf_num = 0; intf_num < HostCtl[ index ].InterfaceNum; intf_num++ )
                            {
                                for( in_num = 0; in_num < HostCtl[ index ].Interface[ intf_num ].InEndpNum; in_num++ )
                                {
                                    /* Get endpoint data based on the interval time of the device */
                                    if( HostCtl[ index ].Interface[ intf_num ].InEndpTimeCount[ in_num ] >= HostCtl[ index ].Interface[ intf_num ].InEndpInterval[ in_num ] )
                                    {
                                        HostCtl[ index ].Interface[ intf_num ].InEndpTimeCount[ in_num ] %= HostCtl[ index ].Interface[ intf_num ].InEndpInterval[ in_num ];

                                        /* Select HUB device port */
                                        USBFSH_SetSelfAddr( RootHubDev[ usb_port ].Device[ hub_port ].bAddress );
                                        USBFSH_SetSelfSpeed( RootHubDev[ usb_port ].Device[ hub_port ].bSpeed );
                                        if( RootHubDev[ usb_port ].bSpeed != USB_LOW_SPEED )
                                        {
                                            USBFSH->HOST_CTRL &= ~USBFS_UH_LOW_SPEED;
                                        }

                                        /* Get endpoint data */
                                        s = USBH_GetHidData( usb_port, index, intf_num, in_num, Com_Buf, &len );
                                        if( s == ERR_SUCCESS )
                                        {
#if DEF_DEBUG_PRINTF
                                            for( i = 0; i < len; i++ )
                                            {
                                                DUG_PRINTF( "%02x ", Com_Buf[ i ] );
                                            }
                                            DUG_PRINTF( "\r\n" );
#endif

                                            if( HostCtl[ index ].Interface[ intf_num ].Type == DEC_KEY )
                                            {
                                                KB_AnalyzeKeyValue( index, intf_num, Com_Buf, len );

                                                if( HostCtl[ index ].Interface[ intf_num ].SetReport_Flag )
                                                {
                                                    KB_SetReport( usb_port, index, RootHubDev[ usb_port ].Device[ hub_port ].bEp0MaxPks, intf_num );
                                                }
                                            }
                                            else if( HostCtl[ index ].Interface[ intf_num ].Type == DEC_MOUSE )
                                            {
                                                USBH_MouseBridge_StoreReport( index, intf_num, Com_Buf, len );
                                            }
                                        }
                                        else if( s == ERR_USB_DISCON )
                                        {
                                            break;
                                        }
                                    }
                                }

                                if( s == ERR_USB_DISCON )
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
#endif
        }
    }
}
