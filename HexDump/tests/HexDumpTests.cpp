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

// Include headers from C modules under test.
extern "C"
{
    #include <CrashCatcher.h>
    #include <CrashCatcherPriv.h>
    #include <DumpMocks.h>

    // Provides the upper 32-bits of 64-bit pointer addresses.
    // When running unit tests on 64-bit machines, the 32-bit emulated PSP an MSP stack pointer addresses don't
    // contain enough bits to make a proper 64-bit pointer.  This provides those upper bits.
    extern uint64_t g_crashCatcherTestBaseAddress;

    // The unit tests can set this to CRASH_CATCHER_EXIT so that HexDump's CrashCatcher_DumpEnd() will cause the Core
    // to exit and not infinite loop.
    extern CrashCatcherReturnCodes g_crashCatcherDumpEndReturn;

    // The unit tests can point the core to a fake location for the SCB->CPUID register.
    extern uint32_t* g_pCrashCatcherCpuId;

    // The unit tests can point the core to a fake location for the fault status registers.
    extern uint32_t* g_pCrashCatcherFaultStatusRegisters;

}
#include <stdarg.h>


// Include C++ headers for test harness.
#include <CppUTest/TestHarness.h>


TEST_GROUP(CrashCatcher)
{
    CrashCatcherExceptionRegisters m_exceptionRegisters;
    uint32_t                       m_emulatedPSP[8];
    uint32_t                       m_emulatedMSP[8];
    uint32_t                       m_emulatedCpuId;
    uint32_t                       m_emulatedFaultStatusRegisters[5];
    uint32_t                       m_expectedSP;
    uint32_t                       m_memoryStart;
    uint8_t                        m_memory[20];
    char                           m_expectedOutput[1025];

    void setup()
    {
        DumpMocks_Init(1024);
        initExceptionRegisters();
        initPSP();
        initMSP();
        initMemory();
        initCpuId();
        initFaultStatusRegisters();
        if (INTPTR_MAX == INT64_MAX)
            g_crashCatcherTestBaseAddress = (uint64_t)&m_emulatedPSP & 0xFFFFFFFF00000000ULL;
        g_crashCatcherDumpEndReturn = CRASH_CATCHER_EXIT;
        m_expectedOutput[0] = '\0';
    }

    void initExceptionRegisters()
    {
        m_exceptionRegisters.exceptionPSR = 0;
        m_exceptionRegisters.psp = (uint32_t)(unsigned long)m_emulatedPSP;
        m_exceptionRegisters.msp = (uint32_t)(unsigned long)m_emulatedMSP;
        m_exceptionRegisters.r4  = 0x44444444;
        m_exceptionRegisters.r5  = 0x55555555;
        m_exceptionRegisters.r6  = 0x66666666;
        m_exceptionRegisters.r7  = 0x77777777;
        m_exceptionRegisters.r8  = 0x88888888;
        m_exceptionRegisters.r9  = 0x99999999;
        m_exceptionRegisters.r10 = 0xAAAAAAAA;
        m_exceptionRegisters.r11 = 0xBBBBBBBB;
        emulateMSPEntry();
    }

    void initPSP()
    {
        m_emulatedPSP[0] = 0xFFFF0000;
        m_emulatedPSP[1] = 0xFFFF1111;
        m_emulatedPSP[2] = 0xFFFF2222;
        m_emulatedPSP[3] = 0xFFFF3333;
        m_emulatedPSP[4] = 0xFFFF4444;
        m_emulatedPSP[5] = 0xFFFF5555;
        m_emulatedPSP[6] = 0xFFFF6666;
        m_emulatedPSP[7] = 0;
    }

    void initMSP()
    {
        m_emulatedMSP[0] = 0x0000FFFF;
        m_emulatedMSP[1] = 0x1111FFFF;
        m_emulatedMSP[2] = 0x2222FFFF;
        m_emulatedMSP[3] = 0x3333FFFF;
        m_emulatedMSP[4] = 0x4444FFFF;
        m_emulatedMSP[5] = 0x5555FFFF;
        m_emulatedMSP[6] = 0x6666FFFF;
        m_emulatedMSP[7] = 0;
    }

    void initMemory()
    {
        for (size_t i = 0 ; i < sizeof(m_memory) ; i++)
            m_memory[i]= i;
        m_memoryStart = (uint32_t)(unsigned long)m_memory;
    }

    void initCpuId()
    {
        static const uint32_t cpuIdCortexM0 = 0x410CC200;
        m_emulatedCpuId = cpuIdCortexM0;
        g_pCrashCatcherCpuId = &m_emulatedCpuId;
    }

    void initFaultStatusRegisters()
    {
        memset(m_emulatedFaultStatusRegisters, 0, sizeof(m_emulatedFaultStatusRegisters));
        g_pCrashCatcherFaultStatusRegisters = m_emulatedFaultStatusRegisters;
    }

    void teardown()
    {
        DumpMocks_Uninit();
    }

    void emulateMSPEntry()
    {
        m_exceptionRegisters.exceptionLR = 0xFFFFFFF1;
        m_expectedSP = (uint32_t)(unsigned long)&m_emulatedMSP + 8 * sizeof(uint32_t);
    }

    void emulatePSPEntry()
    {
        m_exceptionRegisters.exceptionLR = 0xFFFFFFFD;
        m_expectedSP = (uint32_t)(unsigned long)&m_emulatedPSP + 8 * sizeof(uint32_t);
    }

    void emulateStackAlignmentDuringException()
    {
        m_emulatedMSP[7] = 0x200;
        m_expectedSP |= 4;
    }

    void setExpectedRegisterOutput()
    {
        snprintf(m_expectedOutput, sizeof(m_expectedOutput),
                 "\r\n\r\nCRASH ENCOUNTERED\r\n"
                 "Enable logging and then press any key to start dump.\r\n"
                 "\r\n"
                 "63430100\r\n"
                 "%08X%08X%08X%08X\r\n"
                 "%08X%08X%08X%08X\r\n"
                 "%08X%08X%08X%08X\r\n"
                 "%08X\r\n"
                 "%08X\r\n"
                 "%08X%08X%08X\r\n"
                 "%08X\r\n",
                 byteSwap(m_emulatedMSP[0]), byteSwap(m_emulatedMSP[1]), byteSwap(m_emulatedMSP[2]), byteSwap(m_emulatedMSP[3]),
                 byteSwap(m_exceptionRegisters.r4), byteSwap(m_exceptionRegisters.r5), byteSwap(m_exceptionRegisters.r6),
                 byteSwap(m_exceptionRegisters.r7), byteSwap(m_exceptionRegisters.r8), byteSwap(m_exceptionRegisters.r9),
                 byteSwap(m_exceptionRegisters.r10), byteSwap(m_exceptionRegisters.r11),
                 byteSwap(m_emulatedMSP[4]),
                 byteSwap(m_expectedSP),
                 byteSwap(m_emulatedMSP[5]), byteSwap(m_emulatedMSP[6]), byteSwap(m_emulatedMSP[7]),
                 byteSwap(m_exceptionRegisters.exceptionPSR));
    }

    void appendExpectedMemoryOutput(const CrashCatcherMemoryRegion* pRegion, const char* pFormat, ...)
    {
        char memHeaderOutput[128];
        char memDataOutput[128];

        snprintf(memHeaderOutput, sizeof(memHeaderOutput),
                 "%08X%08X\r\n",
                 byteSwap(pRegion->startAddress),
                 byteSwap(pRegion->endAddress));
        strcat(m_expectedOutput, memHeaderOutput);

        va_list list;
        va_start(list, pFormat);
        vsnprintf(memDataOutput, sizeof(memDataOutput), pFormat, list);
        va_end(list);
        strcat(m_expectedOutput, memDataOutput);
    }

    void appendExpectedTrailerOutput()
    {
        strcat(m_expectedOutput, "\r\nEnd of dump\r\n");
    }

    uint32_t byteSwap(uint32_t val)
    {
        return (val << 24) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) | (val >> 24);
    }
};


TEST(CrashCatcher, DumpMultipleRegions)
{
    static const CrashCatcherMemoryRegion regions[] = { {        m_memoryStart,         m_memoryStart + 1, CRASH_CATCHER_BYTE},
                                                        {    m_memoryStart + 1,     m_memoryStart + 1 + 2, CRASH_CATCHER_HALFWORD},
                                                        {m_memoryStart + 1 + 2, m_memoryStart + 1 + 2 + 4, CRASH_CATCHER_WORD},
                                                        {           0xFFFFFFFF,                0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0], "%02X\r\n", m_memory[0]);
    appendExpectedMemoryOutput(&regions[1], "%02X%02X\r\n", m_memory[1], m_memory[2]);
    appendExpectedMemoryOutput(&regions[2], "%02X%02X%02X%02X\r\n", m_memory[3], m_memory[4], m_memory[5], m_memory[6]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}

TEST(CrashCatcher, Dump16Bytes_ShouldFitOnOneLine)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 16, CRASH_CATCHER_BYTE},
                                                        {   0xFFFFFFFF,         0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0],
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               m_memory[0], m_memory[1], m_memory[2], m_memory[3],
                               m_memory[4], m_memory[5], m_memory[6], m_memory[7],
                               m_memory[8], m_memory[9], m_memory[10], m_memory[11],
                               m_memory[12], m_memory[13], m_memory[14], m_memory[15]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}

TEST(CrashCatcher, Dump17Bytes_ShouldSplitAcrossTwoLines)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 17, CRASH_CATCHER_BYTE},
                                                        {   0xFFFFFFFF,         0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0],
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n"
                               "%02X\r\n",
                               m_memory[0], m_memory[1], m_memory[2], m_memory[3],
                               m_memory[4], m_memory[5], m_memory[6], m_memory[7],
                               m_memory[8], m_memory[9], m_memory[10], m_memory[11],
                               m_memory[12], m_memory[13], m_memory[14], m_memory[15],
                               m_memory[16]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}

TEST(CrashCatcher, Dump8HalfWords_ShouldFitOnOneLine)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 16, CRASH_CATCHER_HALFWORD},
                                                        {   0xFFFFFFFF,         0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0],
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               m_memory[0], m_memory[1], m_memory[2], m_memory[3],
                               m_memory[4], m_memory[5], m_memory[6], m_memory[7],
                               m_memory[8], m_memory[9], m_memory[10], m_memory[11],
                               m_memory[12], m_memory[13], m_memory[14], m_memory[15]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}

TEST(CrashCatcher, Dump9HalfWords_ShouldSplitAcrossTwoLines)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 18, CRASH_CATCHER_HALFWORD},
                                                        {   0xFFFFFFFF,         0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0],
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n"
                               "%02X%02X\r\n",
                               m_memory[0], m_memory[1], m_memory[2], m_memory[3],
                               m_memory[4], m_memory[5], m_memory[6], m_memory[7],
                               m_memory[8], m_memory[9], m_memory[10], m_memory[11],
                               m_memory[12], m_memory[13], m_memory[14], m_memory[15],
                               m_memory[16], m_memory[17]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}

TEST(CrashCatcher, Dump4Words_ShouldFitOnOneLine)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 16, CRASH_CATCHER_WORD},
                                                        {   0xFFFFFFFF,         0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0],
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                               m_memory[0], m_memory[1], m_memory[2], m_memory[3],
                               m_memory[4], m_memory[5], m_memory[6], m_memory[7],
                               m_memory[8], m_memory[9], m_memory[10], m_memory[11],
                               m_memory[12], m_memory[13], m_memory[14], m_memory[15]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}

TEST(CrashCatcher, Dump5Words_ShouldSplitAcrossTwoLines)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 20, CRASH_CATCHER_WORD},
                                                        {   0xFFFFFFFF,         0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    static const int keyPress = '\n';

    DumpMocks_SetMemoryRegions(regions);
    DumpMocks_SetGetcData(&keyPress);
        CrashCatcher_Entry(&m_exceptionRegisters);
    setExpectedRegisterOutput();
    appendExpectedMemoryOutput(&regions[0],
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n"
                               "%02X%02X%02X%02X\r\n",
                               m_memory[0], m_memory[1], m_memory[2], m_memory[3],
                               m_memory[4], m_memory[5], m_memory[6], m_memory[7],
                               m_memory[8], m_memory[9], m_memory[10], m_memory[11],
                               m_memory[12], m_memory[13], m_memory[14], m_memory[15],
                               m_memory[16], m_memory[17], m_memory[18], m_memory[19]);
    appendExpectedTrailerOutput();
    STRCMP_EQUAL(m_expectedOutput, DumpMocks_GetPutCData());
}
