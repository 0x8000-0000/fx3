/**
 * @file synchronization.S
 * @brief Implementation of synchronization primitives
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
 * Based on "ARM Synchronization Primitives" article
 * http://infocenter.arm.com/help/topic/com.arm.doc.dht0008a/DHT0008A_arm_synchronization_primitives.pdf
 */

         .file    "synchronization.S"
         .syntax  unified

         .thumb
         .text
         .align   2

         .thumb_func
         .type    fx3_initializeSemaphore, %function
         .code    16
         .global  fx3_initializeSemaphore

fx3_initializeSemaphore:
         .fnstart
         .cantunwind

         MOV      R1, #0
         STR      R1, [R0]
         BX       LR

         .fnend
         .size    fx3_initializeSemaphore, . - fx3_initializeSemaphore

         .thumb_func
         .type    fx3_signalSemaphore, %function
         .code    16
         .global  fx3_signalSemaphore


         .extern  fx3impl_enqueueTaskOnSemaphore
         .extern  fx3impl_wakeupTasksWaitingOnSemaphore

         .thumb_func
         .type    fx3_waitOnSemaphore, %function
         .code    16
         .global  fx3_waitOnSemaphore

fx3_waitOnSemaphore:
         .fnstart
         .cantunwind

         LDREX    R1, [R0]
         CMP      R1, #0               // Test if semaphore is 0
         BEQ      semaphore_blocked
         SUB      R1, #1               // Decrement temporary copy
         STREX    R2, R1, [R0]         // Attempt store-exclusive
         CMP      R2, #0               // Check if store-exclusive succeeded
         BNE      fx3_waitOnSemaphore
         DMB
         MOV      R0, R1               // return count
         BX       LR

semaphore_blocked:
         // WAIT_FOR_UPDATE
         PUSH     {R0,LR}
         BL       fx3impl_enqueueTaskOnSemaphore
         POP      {R0,LR}
         B        fx3_waitOnSemaphore

         .fnend
         .size    fx3_waitOnSemaphore, . - fx3_waitOnSemaphore

         .thumb_func
         .type    fx3_signalSemaphore, %function
         .code    16
         .global  fx3_signalSemaphore

fx3_signalSemaphore:
         .fnstart
         .cantunwind

         LDREX    R1, [R0]
         ADD      R1, #1               // Increment temporary copy
         STREX    R2, R1, [R0]         // Attempt store-exclusive
         CMP      R2, #0               // Check if store-exclusive succeeded
         BNE      fx3_signalSemaphore
         CMP      R0, #1               // Check if incremented from 0
         DMB
         BGE      semaphore_released
         BX       LR

semaphore_released:

         // SIGNAL_UPDATE
         PUSH     {R0,LR}
         BL       fx3impl_wakeupTasksWaitingOnSemaphore
         POP      {R0,LR}

         BX       LR

         .fnend
         .size    fx3_signalSemaphore, . - fx3_signalSemaphore

         .end

