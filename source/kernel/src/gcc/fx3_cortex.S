/**
 * @file fx3_cortex.S
 * @brief Cortex-Mx specific implementation of FX3 functionality
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

         .file     "fx3_cortex.S"
         .syntax   unified

         .thumb
         .text
         .align    2

         .thumb_func
         .type     lst_pushElement, %function
         .code     16
         .global   lst_pushElement
lst_pushElement:
         .fnstart
         .cantunwind

         LDREX    R3, [R0]
         STR      R3, [R1]
         STREX    R2, R1, [R0]
         CMP      R2, #1
         BEQ      lst_pushElement

         BX       LR

         .fnend
         .size    lst_pushElement, . - lst_pushElement

         .thumb_func
         .type     lst_fetchAll, %function
         .code     16
         .global   lst_fetchAll
lst_fetchAll:
         .fnstart
         .cantunwind

         MOVS     R3, #0
         LDREX    R1, [R0]
         STREX    R2, R3, [R0]
         CMP      R2, #1
         BEQ      lst_fetchAll

         MOV      R0, R1
         BX       LR

         .fnend
         .size    lst_fetchAll, . - lst_fetchAll

         .end

