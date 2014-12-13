/* Copyright (C) 2014  Adam Green (https://github.com/adamgreen)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* Implementation of ARMv7-M assembly language code to trap exceptions and call CrashCatcher_Entry(). */
    .text
    .syntax unified
    .arch armv7-m

    .global HardFault_Handler
    .type HardFault_Handler, %function
    .thumb_func
    /* extern "C" void HardFault_Handler(void);
       Called on Hard Fault exception.  Stacks important registers and calls CrashCatcher_Entry().
    */
HardFault_Handler:
    /* Push the following onto the stack (see CrashCatcherExceptionRegisters structure). The g_crashCatcherStack buffer
       is reserved for use as the stack while CrashCatcher is running.
        exceptionPSR
        psp
        msp
        r4
        r5
        r6
        r7
        r8
        r9
        r10
        r11
        exceptionLR */
    mrs     r1, xpsr
    mrs     r2, psp
    mrs     r3, msp
    ldr     sp, =(g_crashCatcherStack + 4 * 256)
    push.w  {r1-r11,lr}

    // Call CrashCatcher_Entry with first argument pointing to registers that were just stacked.
    mov     r0, sp
    bl      CrashCatcher_Entry

    // CrashCatcher_Entry() shouldn't return but if it does, just infinite loop here.
    b       .

    // Let assembler know that we have hit the end of the HardFault_Handler function.
    .pool
    .size   HardFault_Handler, .-HardFault_Handler


    .end