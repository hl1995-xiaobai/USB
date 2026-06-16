/********************************** (C) COPYRIGHT *******************************
 * USBHS HID mouse device on USB2/PB6/PB7.
*******************************************************************************/
#include <string.h>

#include "debug.h"
#include "ch32v30x_usb.h"
#include "usbd_hs_mouse.h"

#define HS_EP0_SIZE                64
#define HS_MOUSE_EP                1
#define HS_MOUSE_REPORT_LEN        4

static const uint8_t hs_device_desc[] =
{
    0x12, USB_DESCR_TYP_DEVICE,
    0x00, 0x02,
    0x00, 0x00, 0x00,
    HS_EP0_SIZE,
    0x86, 0x1A,
    0x01, 0xFE,
    0x00, 0x01,
    0x01, 0x02, 0x03,
    0x01
};

static const uint8_t hs_config_desc[] =
{
    0x09, USB_DESCR_TYP_CONFIG,
    0x22, 0x00,
    0x01,
    0x01,
    0x00,
    0xA0,
    0x32,

    0x09, USB_DESCR_TYP_INTERF,
    0x00,
    0x00,
    0x01,
    USB_DEV_CLASS_HID,
    0x01,
    0x02,
    0x00,

    0x09, USB_DESCR_TYP_HID,
    0x11, 0x01,
    0x00,
    0x01,
    USB_DESCR_TYP_REPORT,
    0x34, 0x00,

    0x07, USB_DESCR_TYP_ENDP,
    0x81,
    0x03,
    0x04, 0x00,
    0x01
};

static const uint8_t hs_qualifier_desc[] =
{
    0x0A, USB_DESCR_TYP_QUALIF,
    0x00, 0x02,
    0x00,
    0x00,
    0x00,
    HS_EP0_SIZE,
    0x01,
    0x00
};

static const uint8_t hs_mouse_report_desc[] =
{
    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x09, 0x01,
    0xA1, 0x00,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x03,
    0x81, 0x02,
    0x75, 0x05,
    0x95, 0x01,
    0x81, 0x01,
    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x03,
    0x81, 0x06,
    0xC0,
    0xC0
};

static const uint8_t hs_lang_desc[] = {0x04, USB_DESCR_TYP_STRING, 0x09, 0x04};
static const uint8_t hs_manu_desc[] = {0x0E, USB_DESCR_TYP_STRING, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0};
static const uint8_t hs_prod_desc[] = {0x1A, USB_DESCR_TYP_STRING, 'B', 0, 'r', 0, 'i', 0, 'd', 0, 'g', 0, 'e', 0, ' ', 0, 'M', 0, 'o', 0, 'u', 0, 's', 0, 'e', 0};
static const uint8_t hs_ser_desc[] = {0x0A, USB_DESCR_TYP_STRING, '0', 0, '0', 0, '0', 0, '1', 0};

__attribute__((aligned(4))) static uint8_t ep0_buf[HS_EP0_SIZE];
__attribute__((aligned(4))) static uint8_t ep1_buf[HS_EP0_SIZE];

static const uint8_t *ctrl_ptr;
static volatile uint16_t ctrl_len;
static volatile uint8_t setup_code;
static volatile uint8_t setup_type;
static volatile uint16_t setup_value;
static volatile uint16_t setup_index;
static volatile uint16_t setup_length;
static volatile uint8_t dev_addr;
static volatile uint8_t dev_config;
static volatile uint8_t ep1_busy;
static volatile uint8_t hid_idle;
static volatile uint8_t hid_protocol;
static volatile uint32_t diag_reset_count;
static volatile uint32_t diag_setup_count;
static volatile uint32_t diag_set_addr_count;
static volatile uint32_t diag_set_config_count;
static volatile uint8_t diag_last_req_type;
static volatile uint8_t diag_last_req;
static volatile uint16_t diag_last_value;
static volatile uint16_t diag_last_index;
static volatile uint16_t diag_last_length;
static volatile uint8_t diag_last_intflag;
static volatile uint8_t diag_last_intst;

void USBHS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

static int8_t sat_i8(int16_t v)
{
    if(v > 127)
    {
        return 127;
    }
    if(v < -127)
    {
        return -127;
    }
    return (int8_t)v;
}

static uint8_t copy_ctrl_data(void)
{
    uint8_t len = (ctrl_len > HS_EP0_SIZE) ? HS_EP0_SIZE : (uint8_t)ctrl_len;

    if(len && ctrl_ptr)
    {
        memcpy(ep0_buf, ctrl_ptr, len);
        ctrl_ptr += len;
        ctrl_len -= len;
    }
    return len;
}

static void hs_endpoint_init(void)
{
    USBHSD->ENDP_CONFIG = USBHS_UEP0_R_EN | USBHS_UEP0_T_EN | USBHS_UEP1_T_EN;
    USBHSD->ENDP_TYPE = 0;
    USBHSD->BUF_MODE = 0;

    USBHSD->UEP0_DMA = (uint32_t)ep0_buf;
    USBHSD->UEP1_TX_DMA = (uint32_t)ep1_buf;
    USBHSD->UEP0_MAX_LEN = HS_EP0_SIZE;
    USBHSD->UEP1_MAX_LEN = HS_EP0_SIZE;

    USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_RES_ACK;
    USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_RES_NAK;
    USBHSD->UEP1_TX_CTRL = USBHS_UEP_T_RES_NAK;
    USBHSD->UEP1_TX_LEN = 0;
    ep1_busy = 0;
}

void USBHSD_Mouse_Init(void)
{
    RCC_USBCLK48MConfig(RCC_USBCLK48MCLKSource_USBPHY);
    RCC_USBHSPLLCLKConfig(RCC_HSBHSPLLCLKSource_HSE);
    RCC_USBHSConfig(RCC_USBPLL_Div2);
    RCC_USBHSPLLCKREFCLKConfig(RCC_USBHSPLLCKREFCLK_4M);
    RCC_USBHSPHYPLLALIVEcmd(ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBHS, ENABLE);

    USBHSD->CONTROL = USBHS_UC_RESET_SIE | USBHS_UC_CLR_ALL;
    Delay_Us(10);
    USBHSD->CONTROL = 0;

    dev_addr = 0;
    dev_config = 0;
    hid_idle = 0;
    hid_protocol = 1;
    diag_reset_count = 0;
    diag_setup_count = 0;
    diag_set_addr_count = 0;
    diag_set_config_count = 0;
    diag_last_req_type = 0;
    diag_last_req = 0;
    diag_last_value = 0;
    diag_last_index = 0;
    diag_last_length = 0;
    diag_last_intflag = 0;
    diag_last_intst = 0;
    hs_endpoint_init();

    USBHSD->DEV_AD = 0;
    USBHSD->INT_FG = 0xFF;
    USBHSD->HOST_CTRL = USBHS_UH_PHY_SUSPENDM;
    USBHSD->CONTROL = USBHS_UC_DMA_EN | USBHS_UC_INT_BUSY | USBHS_UC_SPEED_HIGH;
    USBHSD->INT_EN = USBHS_UIE_SETUP_ACT | USBHS_UIE_TRANSFER | USBHS_UIE_DETECT | USBHS_UIE_SUSPEND;
    USBHSD->CONTROL |= USBHS_UC_DEV_PU_EN;
    NVIC_EnableIRQ(USBHS_IRQn);
    printf("Step 3: USBHS HID mouse device ready\r\n");
}

uint8_t USBHSD_Mouse_IsConfigured(void)
{
    return dev_config != 0;
}

uint8_t USBHSD_Mouse_SendReport(const MouseReport_t *report)
{
    if(!USBHSD_Mouse_IsConfigured() || ep1_busy)
    {
        return 0;
    }

    ep1_buf[0] = report->buttons & 0x07;
    ep1_buf[1] = (uint8_t)sat_i8(report->dx);
    ep1_buf[2] = (uint8_t)sat_i8(report->dy);
    ep1_buf[3] = (uint8_t)sat_i8(report->wheel);

    ep1_busy = 1;
    USBHSD->UEP1_TX_LEN = HS_MOUSE_REPORT_LEN;
    USBHSD->UEP1_TX_CTRL = (USBHSD->UEP1_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK;
    return 1;
}

void USBHSD_Mouse_GetDiag(USBHSD_MouseDiag_t *diag)
{
    diag->reset_count = diag_reset_count;
    diag->setup_count = diag_setup_count;
    diag->set_addr_count = diag_set_addr_count;
    diag->set_config_count = diag_set_config_count;
    diag->addr = dev_addr;
    diag->config = dev_config;
    diag->last_req_type = diag_last_req_type;
    diag->last_req = diag_last_req;
    diag->last_value = diag_last_value;
    diag->last_index = diag_last_index;
    diag->last_length = diag_last_length;
    diag->last_intflag = diag_last_intflag;
    diag->last_intst = diag_last_intst;
}

static void select_descriptor(uint16_t value)
{
    uint8_t desc_type = value >> 8;
    uint8_t desc_index = value & 0xFF;

    ctrl_ptr = 0;
    ctrl_len = 0;

    if(desc_type == USB_DESCR_TYP_DEVICE)
    {
        ctrl_ptr = hs_device_desc;
        ctrl_len = sizeof(hs_device_desc);
    }
    else if(desc_type == USB_DESCR_TYP_CONFIG)
    {
        ctrl_ptr = hs_config_desc;
        ctrl_len = sizeof(hs_config_desc);
    }
    else if(desc_type == USB_DESCR_TYP_HID)
    {
        ctrl_ptr = &hs_config_desc[18];
        ctrl_len = 9;
    }
    else if(desc_type == USB_DESCR_TYP_REPORT)
    {
        ctrl_ptr = hs_mouse_report_desc;
        ctrl_len = sizeof(hs_mouse_report_desc);
    }
    else if(desc_type == USB_DESCR_TYP_QUALIF)
    {
        ctrl_ptr = hs_qualifier_desc;
        ctrl_len = sizeof(hs_qualifier_desc);
    }
    else if(desc_type == USB_DESCR_TYP_STRING)
    {
        if(desc_index == 0)
        {
            ctrl_ptr = hs_lang_desc;
            ctrl_len = hs_lang_desc[0];
        }
        else if(desc_index == 1)
        {
            ctrl_ptr = hs_manu_desc;
            ctrl_len = hs_manu_desc[0];
        }
        else if(desc_index == 2)
        {
            ctrl_ptr = hs_prod_desc;
            ctrl_len = hs_prod_desc[0];
        }
        else if(desc_index == 3)
        {
            ctrl_ptr = hs_ser_desc;
            ctrl_len = hs_ser_desc[0];
        }
    }

    if(ctrl_len > setup_length)
    {
        ctrl_len = setup_length;
    }
}

static uint8_t handle_setup(void)
{
    PUSB_SETUP_REQ req = (PUSB_SETUP_REQ)ep0_buf;
    uint8_t len = 0;
    uint8_t stall = 0;

    setup_type = req->bRequestType;
    setup_code = req->bRequest;
    setup_value = req->wValue;
    setup_index = req->wIndex;
    setup_length = req->wLength;
    diag_last_req_type = setup_type;
    diag_last_req = setup_code;
    diag_last_value = setup_value;
    diag_last_index = setup_index;
    diag_last_length = setup_length;
    ctrl_ptr = 0;
    ctrl_len = 0;

    if((setup_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD)
    {
        switch(setup_code)
        {
            case USB_GET_DESCRIPTOR:
                select_descriptor(setup_value);
                if(ctrl_ptr)
                {
                    len = copy_ctrl_data();
                }
                else
                {
                    stall = 1;
                }
                break;

            case USB_SET_ADDRESS:
                dev_addr = setup_value & USBHS_MASK_USB_ADDR;
                diag_set_addr_count++;
                len = 0;
                break;

            case USB_GET_CONFIGURATION:
                ep0_buf[0] = dev_config;
                len = 1;
                break;

            case USB_SET_CONFIGURATION:
                dev_config = setup_value & 0xFF;
                diag_set_config_count++;
                ep1_busy = 0;
                USBHSD->UEP1_TX_CTRL = USBHS_UEP_T_TOG_DATA0 | USBHS_UEP_T_RES_NAK;
                len = 0;
                break;

            case USB_GET_INTERFACE:
                ep0_buf[0] = 0;
                len = 1;
                break;

            case USB_SET_INTERFACE:
                len = 0;
                break;

            case USB_GET_STATUS:
                ep0_buf[0] = 0;
                ep0_buf[1] = 0;
                len = 2;
                break;

            default:
                stall = 1;
                break;
        }
    }
    else if((setup_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS)
    {
        switch(setup_code)
        {
            case 0x0A:
                hid_idle = setup_value >> 8;
                len = 0;
                break;

            case 0x02:
                ep0_buf[0] = hid_idle;
                len = 1;
                break;

            case 0x0B:
                hid_protocol = setup_value & 0xFF;
                len = 0;
                break;

            case 0x03:
                ep0_buf[0] = hid_protocol;
                len = 1;
                break;

            default:
                stall = 1;
                break;
        }
    }
    else
    {
        stall = 1;
    }

    if(stall)
    {
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_STALL;
        USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_STALL;
    }
    else if(setup_type & 0x80)
    {
        USBHSD->UEP0_TX_LEN = len;
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
        USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_NAK;
    }
    else
    {
        USBHSD->UEP0_TX_LEN = 0;
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
        USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_ACK;
    }

    return stall;
}

void USBHS_IRQHandler(void)
{
    uint8_t intflag = USBHSD->INT_FG;
    uint8_t intst = USBHSD->INT_ST;

    diag_last_intflag = intflag;
    diag_last_intst = intst;

    if(intflag & USBHS_UIF_TRANSFER)
    {
        switch(intst & USBHS_UIS_TOKEN_MASK)
        {
            case USBHS_UIS_TOKEN_IN:
                switch(intst & (USBHS_UIS_TOKEN_MASK | USBHS_UIS_ENDP_MASK))
                {
                    case USBHS_UIS_TOKEN_IN | 0:
                        if(ctrl_len)
                        {
                            uint8_t len = copy_ctrl_data();
                            USBHSD->UEP0_TX_LEN = len;
                            USBHSD->UEP0_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
                            USBHSD->UEP0_TX_CTRL = (USBHSD->UEP0_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK;
                        }
                        else
                        {
                            USBHSD->DEV_AD = (USBHSD->DEV_AD & ~USBHS_MASK_USB_ADDR) | dev_addr;
                            USBHSD->UEP0_TX_CTRL = (USBHSD->UEP0_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
                            USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_ACK;
                        }
                        break;

                    case USBHS_UIS_TOKEN_IN | HS_MOUSE_EP:
                        USBHSD->UEP1_TX_CTRL = (USBHSD->UEP1_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
                        USBHSD->UEP1_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
                        ep1_busy = 0;
                        break;

                    default:
                        break;
                }
                break;

            case USBHS_UIS_TOKEN_OUT:
                USBHSD->UEP0_TX_LEN = 0;
                USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
                break;

            default:
                break;
        }
        USBHSD->INT_FG = USBHS_UIF_TRANSFER;
    }
    else if(intflag & USBHS_UIF_SETUP_ACT)
    {
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_NAK;
        USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_NAK;
        diag_setup_count++;
        handle_setup();
        USBHSD->INT_FG = USBHS_UIF_SETUP_ACT;
    }
    else if(intflag & USBHS_UIF_BUS_RST)
    {
        diag_reset_count++;
        dev_addr = 0;
        dev_config = 0;
        USBHSD->DEV_AD = 0;
        hs_endpoint_init();
        USBHSD->INT_FG = USBHS_UIF_BUS_RST;
    }
    else
    {
        USBHSD->INT_FG = intflag;
    }
}
