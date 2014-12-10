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
    uint64_t g_crashCatcherTestBaseAddress;
}

static const uint8_t g_expectedSignature[4] = {CRASH_CATCHER_SIGNATURE_BYTE0,
                                               CRASH_CATCHER_SIGNATURE_BYTE1,
                                               CRASH_CATCHER_VERSION_MAJOR,
                                               CRASH_CATCHER_VERSION_MINOR};

#define USING_PSP false
#define USING_MSP true

// Include C++ headers for test harness.
#include <CppUTest/TestHarness.h>


TEST_GROUP(CrashCatcher)
{
    CrashCatcherExceptionRegisters m_exceptionRegisters;
    uint32_t                       m_emulatedPSP[8];
    uint32_t                       m_emulatedMSP[8];
    uint32_t                       m_expectedSP;
    uint32_t                       m_memoryStart;
    uint8_t                        m_memory[16];

    void setup()
    {
        DumpMocks_Init();
        initExceptionRegisters();
        initPSP();
        initMSP();
        initMemory();
        if (INTPTR_MAX == INT64_MAX)
            g_crashCatcherTestBaseAddress = (uint64_t)&m_emulatedPSP & 0xFFFFFFFF00000000ULL;
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

    void validateSignatureAndDumpedRegisters(bool usingMSP)
    {
        uint32_t* pSP = usingMSP ? m_emulatedMSP : m_emulatedPSP;
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(0, g_expectedSignature, CRASH_CATCHER_BYTE, sizeof(g_expectedSignature)));
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(1, &pSP[0], CRASH_CATCHER_BYTE, 4 * sizeof(uint32_t)));
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(2, &m_exceptionRegisters.r4, CRASH_CATCHER_BYTE, (11 - 4 + 1) * sizeof(uint32_t)));
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(3, &pSP[4], CRASH_CATCHER_BYTE, sizeof(uint32_t)));
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(4, &m_expectedSP, CRASH_CATCHER_BYTE, sizeof(uint32_t)));
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(5, &pSP[5], CRASH_CATCHER_BYTE, 3 * sizeof(uint32_t)));
        CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(6, &m_exceptionRegisters.exceptionPSR, CRASH_CATCHER_BYTE, sizeof(uint32_t)));
    }
};


TEST(CrashCatcher, DumpRegistersOnly_MSP_StackAlignmentNeeded)
{
    emulateStackAlignmentDuringException();
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(7, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpRegistersOnly_MSP_StackAlignmentNotNeeded)
{
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(7, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpRegistersOnly_PSP_StackAlignmentNotNeeded)
{
    emulatePSPEntry();
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(7, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_PSP);
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpEndReturnTryAgainOnce_ShouldDumpTwice)
{
    DumpMocks_SetDumpEndLoops(1);
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(2, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(14, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_EQUAL(2, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpOneDoubleByteRegion)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 2, CRASH_CATCHER_BYTE},
                                                        {   0xFFFFFFFF,        0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    DumpMocks_SetMemoryRegions(regions);
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(9, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(7, &regions[0], CRASH_CATCHER_BYTE, sizeof(regions[0])));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(8, &m_memory, CRASH_CATCHER_BYTE, 2));
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpOneWordRegion)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 4, CRASH_CATCHER_WORD},
                                                        {   0xFFFFFFFF,        0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    DumpMocks_SetMemoryRegions(regions);
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(9, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(7, &regions[0], CRASH_CATCHER_BYTE, sizeof(regions[0])));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(8, &m_memory, CRASH_CATCHER_WORD, 1));
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpOneHalfwordRegion)
{
    static const CrashCatcherMemoryRegion regions[] = { {m_memoryStart, m_memoryStart + 2, CRASH_CATCHER_HALFWORD},
                                                        {   0xFFFFFFFF,        0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    DumpMocks_SetMemoryRegions(regions);
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(9, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(7, &regions[0], CRASH_CATCHER_BYTE, sizeof(regions[0])));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(8, &m_memory, CRASH_CATCHER_HALFWORD, 1));
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}

TEST(CrashCatcher, DumpMultipleRegions)
{
    static const CrashCatcherMemoryRegion regions[] = { {        m_memoryStart,         m_memoryStart + 1, CRASH_CATCHER_BYTE},
                                                        {    m_memoryStart + 1,     m_memoryStart + 1 + 2, CRASH_CATCHER_HALFWORD},
                                                        {m_memoryStart + 1 + 2, m_memoryStart + 1 + 2 + 4, CRASH_CATCHER_WORD},
                                                        {           0xFFFFFFFF,                0xFFFFFFFF, CRASH_CATCHER_BYTE} };
    DumpMocks_SetMemoryRegions(regions);
    CrashCatcher_Entry(&m_exceptionRegisters);
    CHECK_EQUAL(1, DumpMocks_GetDumpStartCallCount());
    CHECK_EQUAL(13, DumpMocks_GetDumpMemoryCallCount());
    validateSignatureAndDumpedRegisters(USING_MSP);
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(7, &regions[0], CRASH_CATCHER_BYTE, sizeof(regions[0])));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(8, &m_memory, CRASH_CATCHER_BYTE, 1));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(9, &regions[1], CRASH_CATCHER_BYTE, sizeof(regions[1])));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(10, &m_memory[1], CRASH_CATCHER_HALFWORD, 1));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(11, &regions[2], CRASH_CATCHER_BYTE, sizeof(regions[2])));
    CHECK_TRUE(DumpMocks_VerifyDumpMemoryItem(12, &m_memory[3], CRASH_CATCHER_WORD, 1));
    CHECK_EQUAL(1, DumpMocks_GetDumpEndCallCount());
}
