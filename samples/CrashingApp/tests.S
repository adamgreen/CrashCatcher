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
/* Implementation of code to generate various crash scenarios for testing. */
    .text
    .syntax unified
    .arch armv7-m


    .global testMspMultipleOf8
    .type testMspMultipleOf8, %function
    .thumb_func
    /* extern "C" void testMspMultipleOf8(void);
       Uses MSP and has it pointing to an even multiple of 8.
    */
testMspMultipleOf8:
    // Make SP, an even multiple of 8.
    ldr     sp,=0x10001000
    // Load known values into R0-R12
    movs    r0, #0
    movs    r1, #1
    movs    r2, #2
    movs    r3, #3
    movs    r4, #4
    movs    r5, #5
    movs    r6, #6
    movs    r7, #7
    movs    r8, #8
    movs    r9, #9
    movs    r10, #10
    movs    r11, #11
    movs    r12, #12
    // Generate a hard fault by reading from 0xFFFFFFFF
    mvn     lr, #0
    ldr     lr, [lr]
    // CrashCatcher_Entry() shouldn't return but if it does, just infinite loop here.
    b       .
    // Let assembler know that we have hit the end of the function.
    .pool
    .size   testMspMultipleOf8, .-testMspMultipleOf8


    .global testMspNotMultipleOf8
    .type testMspNotMultipleOf8, %function
    .thumb_func
    /* extern "C" void testMspNotMultipleOf8(void);
       Uses MSP and has it not pointing to an even multiple of 8.
    */
testMspNotMultipleOf8:
    // Make SP, not an even multiple of 8.
    ldr     sp,=0x10001004
    // Load known values into R0-R12
    movs    r0, #0
    movs    r1, #1
    movs    r2, #2
    movs    r3, #3
    movs    r4, #4
    movs    r5, #5
    movs    r6, #6
    movs    r7, #7
    movs    r8, #8
    movs    r9, #9
    movs    r10, #10
    movs    r11, #11
    movs    r12, #12
    // Generate a hard fault by reading from 0xFFFFFFFF
    mvn     lr, #0
    ldr     lr, [lr]
    // CrashCatcher_Entry() shouldn't return but if it does, just infinite loop here.
    b       .
    // Let assembler know that we have hit the end of the function.
    .pool
    .size   testMspNotMultipleOf8, .-testMspNotMultipleOf8


    .global testPspMultipleOf8
    .type testPspMultipleOf8, %function
    .thumb_func
    /* extern "C" void testPspMultipleOf8(void);
       Uses MSP and has it pointing to an even multiple of 8.
    */
testPspMultipleOf8:
    // Make PSP, an even multiple of 8.
    ldr     r0,=0x10001000
    msr     psp, r0
    // Switch to use of PSP.
    mov     r0, #3
    msr     control,r0
    // Flush instructions so that control mods take effect.
    isb
    // Load known values into R0-R12
    movs    r0, #0
    movs    r1, #1
    movs    r2, #2
    movs    r3, #3
    movs    r4, #4
    movs    r5, #5
    movs    r6, #6
    movs    r7, #7
    movs    r8, #8
    movs    r9, #9
    movs    r10, #10
    movs    r11, #11
    movs    r12, #12
    // Generate a hard fault by reading from 0xFFFFFFFF
    mvn     lr, #0
    ldr     lr, [lr]
    // CrashCatcher_Entry() shouldn't return but if it does, just infinite loop here.
    b       .
    // Let assembler know that we have hit the end of the function.
    .pool
    .size   testPspMultipleOf8, .-testPspMultipleOf8

    .end