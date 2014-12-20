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
/* Private header file shared with unit tests. */
#ifndef _CRASH_CATCHER_PRIV_H_
#define _CRASH_CATCHER_PRIV_H_

#include <stdint.h>


/* Bit in LR to indicate whether PSP was used for automatic stacking of registers during exception entry. */
#define LR_PSP (1 << 2)


/* Bit in auto stacked xPSR which indicates whether stack was force 8-byte aligned. */
#define PSR_STACK_ALIGN (1 << 9)


/* This magic sentinel value is placed at the bottom of stack and validated to make sure that the stack doesn't overflow
   and overwrite it. */
#define STACK_SENTINEL 0xACCE55ED


/* This structure contains the integer registers that are automatically stacked by Cortex-M processor when it enters
   an exception handler. */
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} CrashCatcherStackedRegisters;


/* This structure is filled in by the Hard Fault exception handler (or unit test) and then passed in as a parameter to
   CrashCatcher_Entry(). */
typedef struct
{
    uint32_t exceptionPSR;
    uint32_t psp;
    uint32_t msp;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t exceptionLR;
} CrashCatcherExceptionRegisters;


// This is the area of memory that would normally be used for the stack when running on an actual Cortex-M
// processor.  Unit tests can write to this buffer to simulate stack overflow.
extern uint32_t g_crashCatcherStack[];


/* The main entry point into CrashCatcher.  Is called from the HardFault exception handler and unit tests. */
void CrashCatcher_Entry(const CrashCatcherExceptionRegisters* pExceptionRegisters);


#endif /* _CRASH_CATCHER_PRIV_H_ */
