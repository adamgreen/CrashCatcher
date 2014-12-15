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
#include <mbed.h>
#include <CrashCatcher.h>


Serial pc(USBTX, USBRX);


// Assembly language routines defined in tests.S
extern "C" void testMspMultipleOf8(void);
extern "C" void testMspNotMultipleOf8(void);
extern "C" void testPspMultipleOf8(void);


static void enable8ByteStackAlignment();


int main()
{
    char buffer[128];
    int option = -1;

    enable8ByteStackAlignment();

    pc.baud(115200);
    while (1)
    {
        printf("\r\n\r\nSelect crash test to run\r\n");
        printf("1) MSP Rounded to multiple of 8 bytes.\r\n");
        printf("2) MSP Not Rounded to multiple of 8 bytes.\r\n");
        printf("3) PSP in use.\r\n");
        printf("Select option: ");
        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &option);

        switch (option)
        {
        case 1:
            testMspMultipleOf8();
            break;
        case 2:
            testMspNotMultipleOf8();
            break;
        case 3:
            testPspMultipleOf8();
            break;
        default:
            continue;
        }
    }
}

static void enable8ByteStackAlignment()
{
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;
}


// Let CrashCatcher know what RAM contents should be part of crash dump.
extern "C" const CrashCatcherMemoryRegion* CrashCatcher_GetMemoryRegions(void)
{
    static const CrashCatcherMemoryRegion regions[] = {
#if defined(TARGET_LPC1768)
                                                        {0x10000000, 0x10008000, CRASH_CATCHER_BYTE},
                                                        {0x2007C000, 0x20084000, CRASH_CATCHER_BYTE},
                                                        {0xFFFFFFFF, 0xFFFFFFFF, CRASH_CATCHER_BYTE}
#elif defined(TARGET_LPC11U24)
                                                        {0x10000000, 0x10002000, CRASH_CATCHER_BYTE},
                                                        {0x20004000, 0x20004800, CRASH_CATCHER_BYTE},
                                                        {0xFFFFFFFF, 0xFFFFFFFF, CRASH_CATCHER_BYTE}
#else
    #error "Target device isn't supported."
#endif
                                                      };
    return regions;
}
