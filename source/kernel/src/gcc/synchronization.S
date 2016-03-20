/**
 * @file
 * @author ARM
 * @copyright ???
 *
 * Based on "ARM Synchronization Primitives" article
 * http://infocenter.arm.com/help/topic/com.arm.doc.dht0008a/DHT0008A_arm_synchronization_primitives.pdf
 */

   .file    "synchronization.S"
   .syntax  unified

   .thumb
   .section "text"
   .align   2

   .set     LOCKED,   1
   .set     UNLOCKED, 0

   .thumb_func
   .type    mutex_acquire, %function
   .global  mutex_acquire
mutex_acquire:
   .fnstart
   .cantunwind

   LDR      r1, =LOCKED

try_again:
   LDREX    r2, [r0]
   CMP      r2, r1               // Test if mutex is locked on unlocked
   BEQ      mutex_locked
   ITT      NE
   STREXNE  r2, r1, [r0]         // Not locked, attempt to lock it
   CMPNE    r2, #1               // Check if Store-Exclusive Failed
   BEQ      try_again
   // Lock aqcuired
   DMB
   BX       lr

mutex_locked:

   // WAIT_FOR_UPDATE
   PUSH     {r0-r3,r7,r12}
   LDR      r7, =yield
   SVC      #0
   POP      {r0-r3,r7,r12}
   B        try_again

   .fnend
   .size    mutex_acquire, . - mutex_acquire

   .thumb_func
   .type    mutex_release, %function
   .global  mutex_release
mutex_release:
   .fnstart
   .cantunwind

   LDR      r1, =UNLOCKED
   DMB
   STR      r1, [r0]

   // SIGNAL_UPDATE
   PUSH     {r0-r3,r7,r12}
   LDR      r7, =released
   SVC      #0
   POP      {r0-r3,r7,r12}

   BX       lr

   .fnend
   .size    mutex_release, . - mutex_release


   .thumb_func
   .type    semaphore_pend, %function
   .global  semaphore_pend
semaphore_pend:
   .fnstart
   .cantunwind

   LDREX    r1, [r0]
   CMP      r1, #0               // Test if semaphore is 0
   BEQ      semaphore_blocked
   SUB      r1, #1               // Decrement temporary copy
   STREX    r2, r1, [r0]         // Attempt store-exclusive
   CMP      r2, #0               // Check if store-exclusive succeeded
   BNE      semaphore_pend
   DMB
   MOV      r0, r1
   BX       lr

semaphore_blocked:
   // WAIT_FOR_UPDATE
   PUSH     {r0-r3,r7,r12}
   LDR      r7, =yield
   SVC      #0
   POP      {r0-r3,r7,r12}
   B        semaphore_pend

   .fnend
   .size    semaphore_pend, . - semaphore_pend

   .thumb_func
   .type    semaphore_post, %function
   .global  semaphore_post
semaphore_post:
   .fnstart
   .cantunwind

   LDREX    r1, [r0]
   ADD      r1, #1               // Increment temporary copy
   STREX    r2, r1, [r0]         // Attempt store-exclusive
   CMP      r2, #0               // Check if store-exclusive succeeded
   BNE      semaphore_post
   CMP      r0, #1               // Check if incremented from 0
   DMB
   BGE      semaphore_released
   BX       lr

semaphore_released:

   // SIGNAL_UPDATE
   PUSH     {r0-r3,r7,r12}
   LDR      r7, =released
   SVC      #0
   POP      {r0-r3,r7,r12}

   BX       lr

   .fnend
   .size    semaphore_post, . - semaphore_post
