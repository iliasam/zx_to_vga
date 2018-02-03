#ifndef _RGB_CAPTURE_H
#define _RGB_CAPTURE_H

#include <stdint.h>

#define RGB_CAPTURE_MAX_SAMPLES         450 //in line
#define RGB_CAPTURE_MAX_LINES           300
#define RGB_CAPTURE_LEFT_OFFSET         60
#define RGB_CAPTURE_IMAGE_WIDTH         (RGB_CAPTURE_MAX_SAMPLES - RGB_CAPTURE_LEFT_OFFSET)

#define RGB_CAPTURE_TIMER_CLK           RCC_APB2Periph_TIM8
#define RGB_CAPTURE_SYNC_PORT_CLK       RCC_AHB1Periph_GPIOB

#define RGB_CAPTURE_SYNC_PORT           GPIOI
#define RGB_CAPTURE_SYNC_PIN            GPIO_Pin_5
#define RGB_CAPTURE_SYNC_SOURCE         GPIO_PinSource5

#define RGB_CAPTURE_FREQUENCY           8000000 //Hz - capture events frequency

#define RGB_CAPTURE_TIM_NAME            TIM8
#define RGB_CAPTURE_TIM_SYNC_AF         GPIO_AF_TIM8
//#define RGB_CAPTURE_TIM_INT_HANDLER     TIM3_IRQn
//#define RGB_CAPTURE_TIM_INT_FUNCION     TIM3_IRQHandler

#define RGB_CAPTURE_TIM_INT_HANDLER     TIM8_CC_IRQn
#define RGB_CAPTURE_TIM_INT_FUNCION     TIM8_CC_IRQHandler

#define RGB_T_MEASURE_TIMER_CLK         RCC_APB1Periph_TIM2
#define RGB_T_MEASURE_TIM_NAME          TIM2
#define RGB_T_MEASURE_FREQUENCY         1000000//Hz - frequency of the timer ticks

#define RGB_MIN_SYNC_PERIOD             40 //in uS - lines with visual data must be longer than that time

#define RGB_CAPTURE_DMA_STREAM          DMA2_Stream2
#define RGB_CAPTURE_DMA_CH              DMA_Channel_0
#define RGB_CAPTURE_DMA_IRQ_CH          DMA2_Stream2_IRQn
#define RGB_CAPTURE_DMA_FUNCION         DMA2_Stream2_IRQHandler

#define RGB_CAPTURE_DMA_ERR_FLAG        DMA_IT_TEIF2
#define RGB_CAPTURE_DMA_TC_FLAG         DMA_IT_TCIF2
#define RGB_CAPTURE_DMA_HT_FLAG         DMA_IT_HTIF2

#define RGB_CAPTURE_DATA_PORT_CLK       RCC_AHB1Periph_GPIOB
#define RGB_CAPTURE_DATA_PORT           GPIOB

#define RGB_CAPTURE_DATA_B_PIN          GPIO_Pin_3
#define RGB_CAPTURE_DATA_G_PIN          GPIO_Pin_4
#define RGB_CAPTURE_DATA_R_PIN          GPIO_Pin_7




void init_capture_timer(void);
void init_time_measurement_timer(void);
void init_capture_dma(void);
void init_data_pins(void);

void capture_dma_config(uint8_t* pointer);

#endif