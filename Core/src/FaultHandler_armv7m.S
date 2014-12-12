/* Copyright 2014 Adam Green (http://mbed.org/users/AdamGreen/)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    ldr     sp, =g_crashCatcherStack
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