/**
 * @file stm32f4_driver_usart.c
 * @brief USART driver implmementation for STM32F4 chips
 * @author Florin Iucha <florin@signbit.net>
 * @copyright Apache License, Version 2.0
 */

/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of FX3 RTOS for ARM Cortex-M4
 */

#include <assert.h>
#include <string.h>

#include <usart.h>

#include <board_local.h>

enum Status usart_initialize(struct USARTHandle* handle, const struct USARTConfiguration* config)
{
   fx3_initializeSemaphore(&handle->receiveBufferNotEmpty, 0);

   // TODO: copy the rest of the settings
   handle->huart.Init.BaudRate     = config->baudRate;
   handle->huart.Init.WordLength   = UART_WORDLENGTH_8B;
   handle->huart.Init.StopBits     = UART_STOPBITS_1;
   handle->huart.Init.Parity       = UART_PARITY_NONE;
   handle->huart.Init.Mode         = UART_MODE_TX_RX;
   handle->huart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
   handle->huart.Init.OverSampling = UART_OVERSAMPLING_16;
   HAL_UART_Init(&handle->huart);

   /*
    * Configure the DMA handler for transmit process
    */

   // TODO: extract channel init value out
   handle->transmitDMA.Init.Channel             = handle->transmitDMAChannel;
   handle->transmitDMA.Init.Direction           = DMA_MEMORY_TO_PERIPH;
   handle->transmitDMA.Init.PeriphInc           = DMA_PINC_DISABLE;
   handle->transmitDMA.Init.MemInc              = DMA_MINC_ENABLE;
   handle->transmitDMA.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
   handle->transmitDMA.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
   handle->transmitDMA.Init.Mode                = DMA_NORMAL;
   handle->transmitDMA.Init.Priority            = DMA_PRIORITY_LOW;
   handle->transmitDMA.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
   handle->transmitDMA.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
   handle->transmitDMA.Init.MemBurst            = DMA_MBURST_INC4;
   handle->transmitDMA.Init.PeriphBurst         = DMA_PBURST_INC4;

   HAL_DMA_Init(&handle->transmitDMA);

   /* Associate the initialized DMA handle to the UART handle */
   //__HAL_LINKDMA(handle->huart, hdmatx, handle->transmitDMA);
   handle->huart.hdmatx       = &handle->transmitDMA;
   handle->transmitDMA.Parent = &handle->huart;

   HAL_NVIC_SetPriority(handle->transmitDMAIRQ, 0, 1);
   // No need to enable here, it will be enabled when there's something to transmit
   //HAL_NVIC_EnableIRQ(handle->transmitDMAIRQ);

   /*
    * Configure the DMA handler for reception process
    */
   handle->receiveDMA.Init.Channel             = handle->receiveDMAChannel;
   handle->receiveDMA.Init.Direction           = DMA_PERIPH_TO_MEMORY;
   handle->receiveDMA.Init.PeriphInc           = DMA_PINC_DISABLE;
   handle->receiveDMA.Init.MemInc              = DMA_MINC_ENABLE;
   handle->receiveDMA.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
   handle->receiveDMA.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
   handle->receiveDMA.Init.Mode                = DMA_CIRCULAR;
   handle->receiveDMA.Init.Priority            = DMA_PRIORITY_HIGH;
   handle->receiveDMA.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
   handle->receiveDMA.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
   handle->receiveDMA.Init.MemBurst            = DMA_MBURST_INC4;
   handle->receiveDMA.Init.PeriphBurst         = DMA_PBURST_INC4;

   HAL_DMA_Init(&handle->receiveDMA);

   /* Associate the initialized DMA handle to the the UART handle */
   //__HAL_LINKDMA(handle->huart, hdmarx, handle->receiveDMA);
   handle->huart.hdmarx      = &handle->receiveDMA;
   handle->receiveDMA.Parent = &handle->huart;

   /*
    * Configure the RX DMA and start it
    */
   HAL_NVIC_SetPriority(handle->receiveDMAIRQ, 0, 0);
   HAL_NVIC_EnableIRQ(handle->receiveDMAIRQ);
   volatile uint32_t* const DR = &(handle->huart.Instance->DR);
   volatile uint32_t* const CR3 = &(handle->huart.Instance->CR3);
   HAL_DMA_Start_IT(handle->huart.hdmarx, (uint32_t) DR, (uint32_t) handle->receiveBuffer.data, handle->receiveBuffer.size);
   *CR3 |= USART_CR3_DMAR;

   /*
    * Enable idle line detection
    */
   volatile uint32_t* const CR1 = &(handle->huart.Instance->CR1);
   *CR1 |= USART_CR1_IDLEIE;

   /* NVIC configuration for USART TC interrupt */
   HAL_NVIC_SetPriority(handle->uartIRQ, 0, 0);
   HAL_NVIC_EnableIRQ(handle->uartIRQ);

   return STATUS_OK;
}

enum Status usart_read(struct USARTHandle* handle, uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesRead)
{
   struct CircularBuffer* receiveBuffer = &handle->receiveBuffer;

   *bytesRead = 0;

   if ((receiveBuffer->head + receiveBuffer->size) < receiveBuffer->tail)
   {
      handle->receiveBufferOverflow ++;
      receiveBuffer->head = receiveBuffer->tail - receiveBuffer->size;
   }

   const uint32_t bytesAvailable = receiveBuffer->tail - receiveBuffer->head;

   if (bufferSize > bytesAvailable)
   {
      bufferSize = bytesAvailable;
   }

   uint32_t realHead = receiveBuffer->head % receiveBuffer->size;
   uint32_t realTail = receiveBuffer->tail % receiveBuffer->size;

   if (realHead > realTail)
   {
      const uint32_t endZoneSize = receiveBuffer->size - realHead;

      memcpy(buffer, handle->receiveBuffer.data + realHead, endZoneSize);

      *bytesRead          += endZoneSize;
      buffer              += endZoneSize;
      bufferSize          -= endZoneSize;

      receiveBuffer->head += endZoneSize;

      realHead   = 0;
   }

   memcpy(buffer, receiveBuffer->data + realHead, bufferSize);
   *bytesRead          += bufferSize;
   receiveBuffer->head += bufferSize;

   return STATUS_OK;
}

enum Status usart_waitForReadable(struct USARTHandle* handle, uint32_t* bytesAvailable)
{
   if (handle->receiveBuffer.tail == handle->receiveBuffer.head)
   {
      fx3_waitOnSemaphore(&handle->receiveBufferNotEmpty);
   }

   if (handle->receiveBuffer.head < handle->receiveBuffer.tail)
   {
      *bytesAvailable = handle->receiveBuffer.tail - handle->receiveBuffer.head;
   }
   else
   {
      *bytesAvailable = handle->receiveBuffer.size + handle->receiveBuffer.head - handle->receiveBuffer.tail - 1;
   }

   return STATUS_OK;
}

static void startTransmit(struct USARTHandle* handle, struct CircularBuffer* transmitBuffer)
{
   handle->transmitStatus.started ++;
   handle->transmitInProgress = true;

   if (transmitBuffer->head < transmitBuffer->tail)
   {
      handle->currentTransmitTail = transmitBuffer->tail;
   }
   else
   {
      handle->currentTransmitTail = transmitBuffer->size;
   }

   uint32_t size = handle->currentTransmitTail - transmitBuffer->head;

   volatile uint32_t* const SR = &(handle->huart.Instance->SR);
   volatile uint32_t* const DR = &(handle->huart.Instance->DR);
   volatile uint32_t* const CR3 = &(handle->huart.Instance->CR3);
   //HAL_DMA_Start_IT(handle->huart.hdmatx, (uint32_t) (transmitBuffer->data + handle->transmitBuffer.head), (uint32_t) DR, size);

   DMA_Stream_TypeDef* txdmaInstance = handle->transmitDMA.Instance;

   /* Disable the peripheral */
   txdmaInstance->CR &= ~DMA_SxCR_EN;

   /* Clear DBM bit */
   txdmaInstance->CR &= ~DMA_SxCR_DBM;

   /* Configure DMA Stream data length */
   txdmaInstance->NDTR = size;

   /* Configure DMA Stream destination address */
   txdmaInstance->PAR = (uint32_t) DR;

   /* Configure DMA Stream source address */
   txdmaInstance->M0AR = (uint32_t) (transmitBuffer->data + handle->transmitBuffer.head);

   // Enable transmit complete interrupt and FIFO errror
   txdmaInstance->CR  |= DMA_IT_TC;
   txdmaInstance->FCR |= DMA_IT_FE;

   /* Enable the Peripheral */
   txdmaInstance->CR |= DMA_SxCR_EN;

   *SR = ~UART_FLAG_TC;
   *CR3 |= USART_CR3_DMAT;
}

enum Status usart_write(struct USARTHandle* handle, const uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesWritten)
{
   struct CircularBuffer* transmitBuffer = &handle->transmitBuffer;
   assert(transmitBuffer->size >= transmitBuffer->head);
   assert(transmitBuffer->size >= transmitBuffer->tail);

   if ((! handle->transmitInProgress) && (bufferSize < 8))
   {
      /*
       * 'putchar' optimization
       */

      volatile uint32_t* const SR = &(handle->huart.Instance->SR);
      volatile uint32_t* const DR = &(handle->huart.Instance->DR);
      const uint32_t inputBufferSize = bufferSize;

      while (bufferSize)
      {
         while (0 == (*SR & UART_FLAG_TXE))
         {
            // wait
         }

         *DR = *buffer;

         buffer ++;
         bufferSize --;
      }

      *bytesWritten = inputBufferSize;

      transmitBuffer->head = 0;
      transmitBuffer->tail = 0;
   }
   else
   {
      if (handle->transmitBufferIsFull)
      {
         return STATUS_FULL;
      }

      if (handle->transmitInProgress)
      {
         // disable to avoid race with completion interrupt
         HAL_NVIC_DisableIRQ(handle->transmitDMAIRQ);
      }

      if (transmitBuffer->head == transmitBuffer->tail)
      {
         assert(! handle->transmitInProgress);

         transmitBuffer->head = 0;

         if (bufferSize > transmitBuffer->size)
         {
            bufferSize = transmitBuffer->size;
         }

         memcpy(transmitBuffer->data, buffer, bufferSize);
         *bytesWritten = bufferSize;

         transmitBuffer->tail = bufferSize;

         handle->transmitBufferIsFull = (bufferSize == transmitBuffer->size);
      }
      else
      {
         *bytesWritten = 0;

         /*
          *  Case 1:    0 ... [HEAD] ... [TAIL] ... [MESSAGE] ... [SIZE]
          *  Case 2:    0 ... [HEAD] ... [TAIL] ... [MESS] ... [SIZE] ... [AGE] ... [SIZE + HEAD]
          *  Case 3:    0 ... [HEAD] ... [TAIL] ... [MESS] ... [SIZE] ... [SIZE + HEAD] ... [AGE]
          *  Case 4:    0 ... [TAIL] ... [MESSAGE] ... [HEAD]
          *  Case 5:    0 ... [TAIL] ... [MESS] ... [HEAD] ... [AGE]
          */
         uint32_t availableSpace = (transmitBuffer->size + transmitBuffer->head) - transmitBuffer->tail;
         if (availableSpace > transmitBuffer->size)
         {
            availableSpace -= transmitBuffer->size;
         }

         if (bufferSize >= availableSpace)
         {
            bufferSize = availableSpace;
            handle->transmitBufferIsFull = true;         // not yet, but soon
         }

         if (transmitBuffer->tail > transmitBuffer->head)
         {
            /*
             * does the message not fit between [tail] and [size]?
             */
            if ((transmitBuffer->tail + bufferSize) > transmitBuffer->size)
            {
               uint32_t endSize = transmitBuffer->size - transmitBuffer->tail;

               memcpy(transmitBuffer->data + transmitBuffer->tail, buffer, endSize);

               buffer        += endSize;
               bufferSize    -= endSize;
               *bytesWritten += endSize;

               transmitBuffer->tail = 0;
            }
         }

         memcpy(transmitBuffer->data + transmitBuffer->tail, buffer, bufferSize);
         *bytesWritten        += bufferSize;
         transmitBuffer->tail += bufferSize;
      }

      if (! handle->transmitInProgress)
      {
         startTransmit(handle, transmitBuffer);
      }

      HAL_NVIC_EnableIRQ(handle->transmitDMAIRQ);

      assert(bufferSize >= *bytesWritten);
   }

   return STATUS_OK;
}

static void usart_onDataAvailable(struct USARTHandle* handle)
{
   fx3_signalSemaphore(&handle->receiveBufferNotEmpty);
}

static void usart_handleIRQ(struct USARTHandle* handle)
{
   volatile uint32_t* const SR  = &(handle->huart.Instance->SR);
   volatile uint32_t* const DR  = &(handle->huart.Instance->DR);
   volatile uint32_t* const CR1 = &(handle->huart.Instance->CR1);

   uint32_t sr = *SR;

   if ((sr & USART_SR_IDLE))
   {
      (void) *DR; // clear idle

      /*
       * Update tail index
       */
      DMA_HandleTypeDef* hdma = &handle->receiveDMA;
      const uint32_t bytesLeft = hdma->Instance->NDTR;

      const uint32_t oldOffset = handle->receiveBuffer.tail % handle->receiveBuffer.size;
      const uint32_t newOffset = handle->receiveBuffer.size - bytesLeft;

      handle->receiveBuffer.tail += newOffset - oldOffset;

      usart_onDataAvailable(handle);
   }

   if ((sr & USART_SR_TC) && (*CR1 & USART_CR1_TCIE))
   {
      // disable TC
      *CR1 &= (~USART_CR1_TCIE) & 0x0000ffff;
   }
}

typedef struct
{
  __IO uint32_t ISR;   /*!< DMA interrupt status register */
  __IO uint32_t Reserved0;
  __IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
} DMA_Base_Registers;

static void usart_handleReceiveDMAIRQ(struct USARTHandle* handle)
{
   /* calculate DMA base and stream number */
   DMA_HandleTypeDef* hdma = &handle->receiveDMA;
   DMA_Base_Registers* regs = (DMA_Base_Registers*) hdma->StreamBaseAddress;

   /* Half Transfer Complete Interrupt management ******************************/
   if ((regs->ISR & (DMA_FLAG_HTIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_HT) != RESET)
      {
         /* Clear the half transfer complete flag */
         regs->IFCR = DMA_FLAG_HTIF0_4 << hdma->StreamIndex;

         /*
          * Update tail index
          */
         const uint32_t oldOffset = handle->receiveBuffer.tail % handle->receiveBuffer.size;
         const uint32_t newOffset = handle->receiveBuffer.size / 2;

         handle->receiveBuffer.tail += newOffset - oldOffset;

         usart_onDataAvailable(handle);
      }
   }

   /* Transfer Complete Interrupt management ***********************************/
   if ((regs->ISR & (DMA_FLAG_TCIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC) != RESET)
      {
         /* Clear the transfer complete flag */
         regs->IFCR = DMA_FLAG_TCIF0_4 << hdma->StreamIndex;

         const uint32_t oldOffset = handle->receiveBuffer.tail % handle->receiveBuffer.size;

         handle->receiveBuffer.tail += handle->receiveBuffer.size - oldOffset;

         usart_onDataAvailable(handle);
      }
   }
}

static void usart_handleTransmitDMAIRQ(struct USARTHandle* handle)
{
   assert(handle->transmitInProgress);

   /* calculate DMA base and stream number */
   DMA_HandleTypeDef* hdma  = &handle->transmitDMA;
   DMA_Base_Registers* regs = (DMA_Base_Registers*) hdma->StreamBaseAddress;

   /*
    * Transfer Error Interrupt
    */
   if ((regs->ISR & (DMA_FLAG_TEIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TE) != RESET)
      {
         /* Disable the transfer error interrupt */
         __HAL_DMA_DISABLE_IT(hdma, DMA_IT_TE);

         /* Clear the transfer error flag */
         regs->IFCR = DMA_FLAG_TEIF0_4 << hdma->StreamIndex;

         // TODO: increment some error counter
      }
   }

   /*
    * FIFO Error Interrupt
    */
   if ((regs->ISR & (DMA_FLAG_FEIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_FE) != RESET)
      {
         /* Disable the FIFO Error interrupt */
         __HAL_DMA_DISABLE_IT(hdma, DMA_IT_FE);

         /* Clear the FIFO error flag */
         regs->IFCR = DMA_FLAG_FEIF0_4 << hdma->StreamIndex;

         // TODO: increment some error counter
      }
   }

   /*
    * Direct Mode Error Interrupt
    */
   if ((regs->ISR & (DMA_FLAG_DMEIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_DME) != RESET)
      {
         /* Disable the direct mode Error interrupt */
         __HAL_DMA_DISABLE_IT(hdma, DMA_IT_DME);

         /* Clear the direct mode error flag */
         regs->IFCR = DMA_FLAG_DMEIF0_4 << hdma->StreamIndex;

         // TODO: increment some error counter
      }
   }

   /*
    * Half Transfer Complete Interrupt
    */
   if ((regs->ISR & (DMA_FLAG_HTIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_HT) != RESET)
      {
         /* Disable the half transfer interrupt */
         __HAL_DMA_DISABLE_IT(hdma, DMA_IT_HT);

         /* Clear the half transfer complete flag */
         regs->IFCR = DMA_FLAG_HTIF0_4 << hdma->StreamIndex;

         // well, ok!
      }
   }

   if ((regs->ISR & (DMA_FLAG_TCIF0_4 << hdma->StreamIndex)) != RESET)
   {
      if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC) != RESET)
      {
         /* Clear the transfer complete flag */
         regs->IFCR = DMA_FLAG_TCIF0_4 << hdma->StreamIndex;

         handle->transmitStatus.completed ++;
         handle->transmitInProgress = false;

         struct CircularBuffer* transmitBuffer = &handle->transmitBuffer;

         if (handle->currentTransmitTail == transmitBuffer->size)
         {
            handle->currentTransmitTail = 0;
         }

         transmitBuffer->head = handle->currentTransmitTail;

         if (transmitBuffer->head != transmitBuffer->tail)
         {
            // there's more data to send
            startTransmit(handle, transmitBuffer);
         }
         else
         {
            HAL_NVIC_DisableIRQ(handle->transmitDMAIRQ);
         }
      }
   }
}

/*
 * Board-dependent portion
 */

extern struct USARTHandle usart1;
extern struct USARTHandle usart2;

void USART1_IRQHandler(void)
{
   usart_handleIRQ(&usart1);
}

// USART1 transmit DMA
void DMA2_Stream7_IRQHandler(void)
{
   usart_handleTransmitDMAIRQ(&usart1);
}

// USART1 receive DMA
void DMA2_Stream2_IRQHandler(void)
{
   usart_handleReceiveDMAIRQ(&usart1);
}

void USART2_IRQHandler(void)
{
   usart_handleIRQ(&usart2);
}

// USART2 transmit DMA
void DMA1_Stream6_IRQHandler(void)
{
   usart_handleTransmitDMAIRQ(&usart2);
}

// USART2 receive DMA
void DMA1_Stream5_IRQHandler(void)
{
   usart_handleReceiveDMAIRQ(&usart2);
}

