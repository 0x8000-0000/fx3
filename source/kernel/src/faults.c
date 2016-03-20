/**
 * @file faults.c
 * @brief Cortex-M4 Fault Handlers
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
 *
 * Based on the example code by Joseph Yiu
 * "The Definitive Guide to ARM® Cortex®-M3 and Cortex®-M4 Processors, Third Edition"
 * Chapter 12
 */

#include <stdint.h>
#include <string.h>

#include <board.h>
#include <board_local.h>

static volatile struct FaultRegisters
{
   uint32_t r0;
   uint32_t r1;
   uint32_t r2;
   uint32_t r3;
   uint32_t r12;
   uint32_t lr;
   uint32_t pc;
   uint32_t psr;

   uint32_t cfsr;
   uint32_t busFaultAddress;
   uint32_t memmanageFaultAddress;

   uint32_t exceptionLr;

}  faultRegisters;

void HardFault_Handler_C(const uint32_t* hardfault_args, uint32_t lrValue)
{
   faultRegisters.cfsr                  = SCB->CFSR;
   if ((faultRegisters.cfsr & 0x0080))
   {
      faultRegisters.memmanageFaultAddress = SCB->MMFAR;
   }
   else
   {
      faultRegisters.memmanageFaultAddress = 0;
   }
   if ((faultRegisters.cfsr & 0x8000))
   {
      faultRegisters.busFaultAddress = SCB->BFAR;
   }
   else
   {
      faultRegisters.busFaultAddress = 0;
   }

   faultRegisters.r0 =  hardfault_args[0];
   faultRegisters.r1 =  hardfault_args[1];
   faultRegisters.r2 =  hardfault_args[2];
   faultRegisters.r3 =  hardfault_args[3];
   faultRegisters.r12 = hardfault_args[4];
   faultRegisters.lr  = hardfault_args[5];
   faultRegisters.pc  = hardfault_args[6];
   faultRegisters.psr = hardfault_args[7];

   faultRegisters.exceptionLr = lrValue;

   if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk))
   {
      __BKPT(0x42);
   }

   while(1)
   {
      ; // endless loop
   }
}

static struct AssertData
{
   char fileName[256];
   int  lineNumber;
   char functionName[64];
   char assertText[256];

}  assertData;

__attribute__((noreturn)) void __assert_func(const char* fileName, int lineNumber, const char* functionName, const char* assertText)
{
   strncpy(assertData.fileName, fileName, sizeof(assertData.fileName));
   assertData.lineNumber = lineNumber;
   strncpy(assertData.functionName, functionName, sizeof(assertData.functionName));
   strncpy(assertData.assertText, assertText, sizeof(assertData.assertText));

   bsp_reset();
}

