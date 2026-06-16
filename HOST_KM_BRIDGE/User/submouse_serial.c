/********************************** (C) COPYRIGHT *******************************
 * Top Type-C WCH-LinkE virtual COM input on USART1 PA9/PA10.
 *
 * Frame:
 *   A5 5A seq buttons dx_lo dx_hi dy_lo dy_hi wheel checksum
 * checksum is the low 8 bits of bytes 0..8.
*******************************************************************************/
#include "debug.h"
#include "submouse_serial.h"

#define SUBMOUSE_FRAME_LEN       10
#define SUBMOUSE_RX_BUF_SIZE     128

static volatile uint8_t rx_buf[SUBMOUSE_RX_BUF_SIZE];
static volatile uint8_t rx_w;
static volatile uint8_t rx_r;
static MouseReport_t pending_report;
static uint8_t pending_valid;
static uint16_t bad_checksum_count;

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

static void usart1_send_text(const char *text)
{
    while(*text)
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        {
        }
        USART_SendData(USART1, *text++);
    }
}

static int16_t read_i16_le(uint8_t lo, uint8_t hi)
{
    return (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
}

static uint8_t rx_pop(uint8_t *data)
{
    if(rx_r == rx_w)
    {
        return 0;
    }

    *data = rx_buf[rx_r];
    rx_r = (uint8_t)((rx_r + 1) % SUBMOUSE_RX_BUF_SIZE);
    return 1;
}

void SubMouseSerial_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);

    printf("Step 2: top Type-C USART1 PA9/PA10 ready, baud:%d\r\n", baudrate);
}

void SubMouseSerial_SendAlive(void)
{
    usart1_send_text("USART1 alive\r\n");
}

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t next = (uint8_t)((rx_w + 1) % SUBMOUSE_RX_BUF_SIZE);
        uint8_t data = (uint8_t)(USART_ReceiveData(USART1) & 0xFF);

        if(next != rx_r)
        {
            rx_buf[rx_w] = data;
            rx_w = next;
        }
    }
}

void SubMouseSerial_Task(void)
{
    static uint8_t frame[SUBMOUSE_FRAME_LEN];
    static uint8_t pos;
    uint8_t data;

    while(rx_pop(&data))
    {
        if(pos == 0)
        {
            if(data != 0xA5)
            {
                continue;
            }
            frame[pos++] = data;
        }
        else if(pos == 1)
        {
            if(data != 0x5A)
            {
                pos = (data == 0xA5) ? 1 : 0;
                frame[0] = 0xA5;
                continue;
            }
            frame[pos++] = data;
        }
        else
        {
            frame[pos++] = data;
            if(pos == SUBMOUSE_FRAME_LEN)
            {
                uint8_t sum = 0;
                uint8_t i;

                for(i = 0; i < SUBMOUSE_FRAME_LEN - 1; i++)
                {
                    sum = (uint8_t)(sum + frame[i]);
                }

                if(sum == frame[SUBMOUSE_FRAME_LEN - 1])
                {
                    pending_report.dx = read_i16_le(frame[4], frame[5]);
                    pending_report.dy = read_i16_le(frame[6], frame[7]);
                    pending_report.wheel = (int8_t)frame[8];
                    pending_report.buttons = frame[3] & 0x07;
                    pending_report.valid = 1;
                    pending_valid = 1;

                    printf("SUB ok seq:%u btn:%02x dx:%d dy:%d wh:%d\r\n",
                           frame[2],
                           pending_report.buttons,
                           pending_report.dx,
                           pending_report.dy,
                           pending_report.wheel);
                }
                else
                {
                    bad_checksum_count++;
                    printf("SUB checksum bad:%u got:%02x calc:%02x\r\n",
                           bad_checksum_count, frame[9], sum);
                }

                pos = 0;
            }
        }
    }
}

uint8_t SubMouseSerial_ReadReport(MouseReport_t *report)
{
    if(!pending_valid)
    {
        return 0;
    }

    *report = pending_report;
    pending_report.dx = 0;
    pending_report.dy = 0;
    pending_report.wheel = 0;
    pending_report.valid = 0;
    pending_valid = 0;
    return 1;
}
