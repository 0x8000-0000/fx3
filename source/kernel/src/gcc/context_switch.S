/**
 * @file context_switch.S
 * @brief Cortex-Mx Context Switch
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
 * Chapter 10
 */

         .file     "context_switch.S"
         .syntax   unified

         .bss
         .lcomm svcExceptionReturn, 4

         .thumb
         .text
         .align    2

         .thumb_func
         .type     SVC_Handler, %function
         .code     16
         .global   SVC_Handler
SVC_Handler:
         .fnstart
         .cantunwind

         TST      LR, #4   // Extract stack frame location
         ITE      EQ
         MRSEQ    R0, MSP
         MRSNE    R0, PSP
         LDR      R1, =svcExceptionReturn   // Save current EXC_RETURN
         STR      LR, [R1]
         BL       SVC_Handler_C             // Run C part of SVC_Handler
         LDR      R1, =svcExceptionReturn   // Load new EXC_RETURN
         LDR      LR, [R1]
         BX       LR

         .fnend
         .size    SVC_Handler, . - SVC_Handler

         .thumb_func
         .type     PendSV_Handler, %function
         .code     16
         .global   PendSV_Handler
PendSV_Handler:
         .fnstart
         .cantunwind

         CPSID    I                          // disable interrupts while we switch tasks

         PUSH     {LR}
         BL       fx3_selectNextRunningTask
         POP      {LR}

         MRS      R0, PSP                    // Get current process stack pointer
         TST      LR, #0x10                  // Test bit 4. If 0, need to stack floating point registers
         IT       EQ
         VSTMDBEQ R0!, {S16-S31}             // Save floating point registers
         MOV      R2, LR
         MRS      R3, CONTROL
         STMDB    R0!, {R2-R11}              // Save LR, CONTROL and R4 to R11 in task stack (10 regs)
         LDR      R1, =.runningTask
         LDR      R1, [R1]                   // Get current task
         LDR      R2, [R1]
         STR      R0, [R2, #4]               // Save PSP value into runningTask->stackPointer

         // next task

         LDR      R4, =.nextRunningTask
         LDR      R4, [R4]                   // Get next task
         LDR      R4, [R4]
         STR      R4, [R1]                   // Set curr_task = next_task
         LDR      R0, [R4, #4]               // Load PSP value from nextRunningTask->stackPointer
         LDMIA    R0!, {R2-R11}              // Load LR, CONTROL and R4 to R11 from task stack (10 regs)
         MOV      LR, R2
         MSR      CONTROL, R3
         TST      LR, #0x10                  // Test bit 4. If zero, need to unstack floating point regs
         IT       EQ
         VLDMIAEQ R0!, {S16-S31}             // Restore floating point registers
         MSR      PSP, R0                    // Set PSP to next task

         CPSIE    I                          // re-enalbe interrupts
         ISB

         BX       LR                         // Return

         .fnend
         .size   PendSV_Handler, . - PendSV_Handler

.runningTask:
         .word    runningTask
.nextRunningTask:
         .word    nextRunningTask

         .thumb_func
         .type     fx3_startMultitaskingImpl, %function
         .code     16
         .global   fx3_startMultitaskingImpl
fx3_startMultitaskingImpl:
         .fnstart
         .cantunwind

         MSR   PSP, R0
         MOVS  R3, #3
         MSR   CONTROL, R3
         ISB
         MOV   R0, R2
         BX    R1

         .fnend
         .size    fx3_startMultitaskingImpl, . - fx3_startMultitaskingImpl
