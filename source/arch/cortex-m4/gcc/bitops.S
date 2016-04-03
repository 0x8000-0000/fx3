/**
 * @file bitops.S
 * @brief Cortex-Mx Bit Operations
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

         .file     "bitops.S"
         .syntax   unified

         .thumb
         .text
         .align    2

         .thumb_func
         .type     bit_init, %function
         .code     16
         .global   bit_init
bit_init:
         .fnstart
         .cantunwind

         MOVS     R3, #1
         LSL      R3, R3, R1              // R3 <- mask with R1't bit 1
         SUB      R3, #1

         STR      R3, [R0]
         BX       LR

         .fnend
         .size    bit_alloc, . - bit_alloc


         .thumb_func
         .type     bit_alloc, %function
         .code     16
         .global   bit_alloc
bit_alloc:
         .fnstart
         .cantunwind

         MOV      R1, #0
         LDREX    R2, [R0]                   // R2 <- bitmap
         CMP      R2, R1
         BEQ      bitmap_full

         CLZ      R1, R2
         RSB      R1, R1, #31                // R1 <- first non-zero byte
         MOVS     R3, #1
         LSLS     R3, R1                     // R3 <- mask
         BIC      R2, R2, R3                 // masked R1'th bit in bitmap R2

         STREX    R3, R2, [R0]               // bitmap -> R0
         CMP      R3, #1
         BEQ      bit_alloc

         MOV      R0, R1
         BX       LR

bitmap_full:
         MOV      R0, #32
         BX       LR

         .fnend
         .size    bit_alloc, . - bit_alloc

         .thumb_func
         .type     bit_free, %function
         .code     16
         .global   bit_free
bit_free:
         .fnstart
         .cantunwind

         MOVS     R3, #1
         LSL      R3, R3, R1              // R3 <- mask with R1't bit 1

         LDREX    R2, [R0]
         ORRS     R2, R3
         STREX    R3, R2, [R0]
         CMP      R3, #1
         BEQ      bit_free

         BX       LR

         .fnend
         .size    bit_free, . - bit_free

         .end
