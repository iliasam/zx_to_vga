#include "rgb_capture.h"
#include "main.h"

volatile uint16_t last_sync_period = 0;//line duration in uS - from rising to falling edge

//Flags that shows state of the captured line
volatile uint8_t prev_line_is_visual = 0;
volatile uint8_t current_line_is_visual = 0;

volatile uint16_t current_line_number = 0;
volatile uint16_t lines_in_frame = 0;//Number of lines in frame - debug

volatile uint8_t capture_buf0[RGB_CAPTURE_MAX_SAMPLES];
volatile uint8_t capture_buf1[RGB_CAPTURE_MAX_SAMPLES];
volatile uint8_t current_buf_capturing = 0;

volatile uint8_t new_line_is_captured = 0;//flag for external software
volatile uint16_t captured_line_number = 0;

void init_additional_timer(void);

void RGB_CAPTURE_TIM_INT_FUNCION(void);
void RGB_CAPTURE_DMA_FUNCION(void);

//edge of sync input
void RGB_CAPTURE_TIM_INT_FUNCION(void)
{
  //Rising edge
  if (TIM_GetITStatus(RGB_CAPTURE_TIM_NAME, TIM_IT_CC1) != RESET)
  {
    TIM_ClearITPendingBit(RGB_CAPTURE_TIM_NAME, TIM_IT_CC1);
    RGB_T_MEASURE_TIM_NAME->CNT = 0;
  }
  
  //Falling edge
  if (TIM_GetITStatus(RGB_CAPTURE_TIM_NAME, TIM_IT_CC2) != RESET)
  {
    //Reset main capture timer.
    //It would not count up till risign edge (it is confugured in gated mode)
    RGB_CAPTURE_TIM_NAME->CNT = 0;
    last_sync_period = RGB_T_MEASURE_TIM_NAME->CNT;
    TIM_ClearITPendingBit(RGB_CAPTURE_TIM_NAME, TIM_IT_CC2);
    DMA_Cmd(RGB_CAPTURE_DMA_STREAM, DISABLE);
    DMA_ClearITPendingBit(RGB_CAPTURE_DMA_STREAM, RGB_CAPTURE_DMA_TC_FLAG);//dma
    
    if (last_sync_period < RGB_MIN_SYNC_PERIOD)
    {
      //Vertical Blanking Interval detected
      current_line_is_visual = 0;
      if ((prev_line_is_visual == 1) && (current_line_is_visual == 0))//End of the frame detected
      {
        lines_in_frame = current_line_number;
      }
      current_line_number = 0;//this lines dont't count
    }
    else
    {
      current_line_is_visual = 1;
      
      if (current_line_number < RGB_CAPTURE_MAX_LINES)
      {
        GPIOI->ODR^= GPIO_Pin_6;//testing - led 1
        
        if (current_line_number % 1)
          capture_dma_config((uint8_t*)&capture_buf1[0]);
        else
          capture_dma_config((uint8_t*)&capture_buf0[0]);
        asm("nop");//debug
      }
    }
    
    prev_line_is_visual = current_line_is_visual;
    
    current_line_number++;
    

  }
    
}

void RGB_CAPTURE_DMA_FUNCION(void)
{
  if(DMA_GetITStatus(RGB_CAPTURE_DMA_STREAM, RGB_CAPTURE_DMA_TC_FLAG))
  {
    DMA_ClearITPendingBit(RGB_CAPTURE_DMA_STREAM, RGB_CAPTURE_DMA_TC_FLAG);
    GPIOI->ODR^= GPIO_Pin_7;//testing - led 2
    
     new_line_is_captured = 1;
     captured_line_number = current_line_number;
  }
}

void init_capture_timer(void)
{
  GPIO_InitTypeDef              GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef       TIM_TimeBaseStructure;
  TIM_ICInitTypeDef             TIM_ICInitStructure;
  NVIC_InitTypeDef              NVIC_InitStructure;
  
  //RCC_APB1PeriphClockCmd(RGB_CAPTURE_TIMER_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(RGB_CAPTURE_TIMER_CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(RGB_CAPTURE_SYNC_PORT_CLK, ENABLE);
  
  //Sync pin
  GPIO_InitStructure.GPIO_Pin   = RGB_CAPTURE_SYNC_PIN;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL ;
  GPIO_Init(RGB_CAPTURE_SYNC_PORT, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(RGB_CAPTURE_SYNC_PORT, RGB_CAPTURE_SYNC_SOURCE, RGB_CAPTURE_TIM_SYNC_AF);

  // Time base configuration
  TIM_TimeBaseStructure.TIM_Period = (SystemCoreClock) / RGB_CAPTURE_FREQUENCY - 1;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(RGB_CAPTURE_TIM_NAME, &TIM_TimeBaseStructure);
  
  //Ch1 input is used to reset timer
  TIM_ICInitStructure.TIM_Channel       = TIM_Channel_1;
  TIM_ICInitStructure.TIM_ICPolarity    = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection   = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler   = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(RGB_CAPTURE_TIM_NAME, &TIM_ICInitStructure);
  
 // TIM_SelectSlaveMode(RGB_CAPTURE_TIM_NAME, TIM_SlaveMode_Reset);
  TIM_SelectSlaveMode(RGB_CAPTURE_TIM_NAME, TIM_SlaveMode_Gated);
  TIM_SelectInputTrigger(RGB_CAPTURE_TIM_NAME, TIM_TS_TI1FP1);
  //TIM_SelectMasterSlaveMode(RGB_CAPTURE_TIM_NAME, TIM_MasterSlaveMode_Enable);
  
  TIM_DMACmd(RGB_CAPTURE_TIM_NAME, TIM_DMA_CC3, ENABLE);
  
  //Ch2 input is used to detect time of fallind edges
  TIM_ICInitStructure.TIM_Channel       = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity    = TIM_ICPolarity_Falling;
  TIM_ICInitStructure.TIM_ICSelection   = TIM_ICSelection_IndirectTI;//Connect to CH1 input
  TIM_ICInitStructure.TIM_ICPrescaler   = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(RGB_CAPTURE_TIM_NAME, &TIM_ICInitStructure);
  
  // PWM1 Mode configuration: Channel3 - no output
  TIM_OCInitTypeDef             TIM_OCInitStructure;
  TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
  TIM_OCInitStructure.TIM_Pulse       = TIM_TimeBaseStructure.TIM_Period *1/5;
  TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
  TIM_OC3Init(RGB_CAPTURE_TIM_NAME, &TIM_OCInitStructure);
  TIM_OC3PreloadConfig(RGB_CAPTURE_TIM_NAME, TIM_OCPreload_Enable);
  TIM_SelectOutputTrigger(RGB_CAPTURE_TIM_NAME, TIM_TRGOSource_OC3Ref);

  TIM_ClearITPendingBit(RGB_CAPTURE_TIM_NAME, TIM_IT_CC1);
  TIM_ClearITPendingBit(RGB_CAPTURE_TIM_NAME, TIM_IT_CC2);
  TIM_ITConfig(RGB_CAPTURE_TIM_NAME, TIM_IT_CC1, ENABLE);
  TIM_ITConfig(RGB_CAPTURE_TIM_NAME, TIM_IT_CC2, ENABLE);
  
  NVIC_InitStructure.NVIC_IRQChannel                    = RGB_CAPTURE_TIM_INT_HANDLER;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  TIM_ARRPreloadConfig(RGB_CAPTURE_TIM_NAME, ENABLE);
  TIM_Cmd(RGB_CAPTURE_TIM_NAME, ENABLE);
  
  //init_additional_timer();
}

//DMA transfer data from GPIO to memory
void init_capture_dma(void)
{
  DMA_InitTypeDef DMA_InitStructure;
  NVIC_InitTypeDef              NVIC_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  
  /* DMA1 Stream4 channel5 configuration */
  DMA_InitStructure.DMA_Channel                 = RGB_CAPTURE_DMA_CH;
  DMA_InitStructure.DMA_PeripheralBaseAddr      = (uint32_t)&GPIOB->IDR;
  DMA_InitStructure.DMA_Memory0BaseAddr         = (uint32_t)&capture_buf0[0];
  DMA_InitStructure.DMA_DIR                     = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize              = RGB_CAPTURE_MAX_SAMPLES;
  DMA_InitStructure.DMA_PeripheralInc           = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc               = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize      = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize          = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode                    = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority                = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode                = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold           = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst             = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst         = DMA_PeripheralBurst_Single;
  DMA_Init(RGB_CAPTURE_DMA_STREAM, &DMA_InitStructure);
  
  //for debug
  NVIC_InitStructure.NVIC_IRQChannel = RGB_CAPTURE_DMA_IRQ_CH;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  DMA_ITConfig(RGB_CAPTURE_DMA_STREAM, DMA_IT_TC, ENABLE);
}

//Configure DMA for new capture
void capture_dma_config(uint8_t* pointer)
{ 
  DMA_Cmd(RGB_CAPTURE_DMA_STREAM, DISABLE);
  RGB_CAPTURE_DMA_STREAM->NDTR = RGB_CAPTURE_MAX_SAMPLES;
  RGB_CAPTURE_DMA_STREAM->M0AR = (uint32_t)pointer;
  
  DMA_ClearITPendingBit(RGB_CAPTURE_DMA_STREAM, RGB_CAPTURE_DMA_ERR_FLAG);
  DMA_ClearITPendingBit(RGB_CAPTURE_DMA_STREAM, RGB_CAPTURE_DMA_TC_FLAG);
  DMA_ClearITPendingBit(RGB_CAPTURE_DMA_STREAM, RGB_CAPTURE_DMA_HT_FLAG);
  DMA_Cmd(RGB_CAPTURE_DMA_STREAM, ENABLE);
}

//Timer is used to measure short periods of time
void init_time_measurement_timer(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure); 
  
  RCC_APB1PeriphClockCmd(RGB_T_MEASURE_TIMER_CLK, ENABLE);
 
  TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 2) / RGB_T_MEASURE_FREQUENCY - 1;
  TIM_TimeBaseStructure.TIM_Period = 0xffff;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(RGB_T_MEASURE_TIM_NAME, &TIM_TimeBaseStructure); 

  TIM_Cmd(RGB_T_MEASURE_TIM_NAME, ENABLE);
}

void init_data_pins(void)
{
  GPIO_InitTypeDef              GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RGB_CAPTURE_DATA_PORT_CLK, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin   = RGB_CAPTURE_DATA_R_PIN | RGB_CAPTURE_DATA_G_PIN | RGB_CAPTURE_DATA_B_PIN;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP ;
  GPIO_Init(RGB_CAPTURE_DATA_PORT, &GPIO_InitStructure);
}