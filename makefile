# Copyright 2014 Adam Green (https://github.com/adamgreen)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# User can set VERBOSE variable to have all commands echoed to console for debugging purposes.
ifdef VERBOSE
    Q :=
else
    Q := @
endif

# *** High Level Make Rules ***
.PHONY : arm clean host all gcov

arm : ARM_LIBS

# UNDONE: Maybe I can just add to a list automatically
host : RUN_CPPUTEST_TESTS RUN_CORE_TESTS

all : arm host

# UNDONE: Maybe I can just add to a list automatically
gcov : RUN_CPPUTEST_TESTS GCOV_CORE

clean :
	@echo Cleaning CrashCatcher
	$Q $(REMOVE_DIR) $(OBJDIR) $(QUIET)
	$Q $(REMOVE_DIR) $(LIBDIR) $(QUIET)
	$Q $(REMOVE_DIR) $(GCOVDIR) $(QUIET)
	$Q $(REMOVE) *_tests$(EXE) $(QUIET)
	$Q $(REMOVE) *_tests_gcov$(EXE) $(QUIET)


#  Names of tools for cross-compiling ARMv7-M binaries.
ARM_GCC := arm-none-eabi-gcc
ARM_GPP := arm-none-eabi-g++
ARM_AS  := arm-none-eabi-gcc
ARM_LD  := arm-none-eabi-g++
ARM_AR  := arm-none-eabi-ar

#  Names of tools for compiling binaries to run on this host system.
HOST_GCC := gcc
HOST_GPP := g++
HOST_AS  := gcc
HOST_LD  := g++
HOST_AR  := ar

# Handle Windows and *nix differences.
ifeq "$(OS)" "Windows_NT"
    MAKEDIR = mkdir $(subst /,\,$(dir $@))
    REMOVE := del /q
    REMOVE_DIR := rd /s /q
    QUIET := >nul 2>nul & exit 0
    EXE := .exe
else
    MAKEDIR = mkdir -p $(dir $@)
    REMOVE := rm
    REMOVE_DIR := rm -r -f
    QUIET := > /dev/null 2>&1 ; exit 0
    EXE :=
endif

# UNDONE: -mcpu=cortex-m3
# Flags to use when cross-compiling ARM binaries.
ARM_GCCFLAGS := -Os -g3 -mthumb -mthumb-interwork -Wall -Wextra -Werror -Wno-unused-parameter -MMD -MP
ARM_GCCFLAGS += -ffunction-sections -fdata-sections -fno-exceptions -fno-delete-null-pointer-checks -fomit-frame-pointer
ARM_GPPFLAGS := $(ARM_GCCFLAGS) -fno-rtti
ARM_GCCFLAGS += -std=gnu90
ARM_LDFLAGS   = -mthumb -Wl,-Map=$(basename $@).map,--cref,--gc-sections
ARM_ASFLAGS  := -mthumb -x assembler-with-cpp -MMD -MP

# Flags to use when compiling binaries to run on this host system.
HOST_GCCFLAGS := -O0 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -MMD -MP
HOST_GCCFLAGS += -ffunction-sections -fdata-sections -fno-common
HOST_GCCFLAGS += -include CppUTest/include/CppUTest/MemoryLeakDetectorMallocMacros.h
HOST_GPPFLAGS := $(HOST_GCCFLAGS) -include CppUTest/include/CppUTest/MemoryLeakDetectorNewMacros.h
HOST_GCCFLAGS += -std=gnu90
HOST_ASFLAGS  := -x assembler-with-cpp -MMD -MP

# Output directories for intermediate object files.
OBJDIR        := obj
ARMV6M_OBJDIR := $(OBJDIR)/armv6-m
ARMV7M_OBJDIR := $(OBJDIR)/armv7-m
HOST_OBJDIR   := $(OBJDIR)/host

# Output directory for gcov files.
GCOVDIR := gcov

# Output directories for final libraries.
LIBDIR        := lib
ARMV6M_LIBDIR := $(LIBDIR)/armv6-m
ARMV7M_LIBDIR := $(LIBDIR)/armv7-m
HOST_LIBDIR   := $(LIBDIR)/host

# Customize some variables for code coverage builds.
GCOV_HOST_OBJDIR        := $(GCOVDIR)/$(HOST_OBJDIR)
GCOV_HOST_LIBDIR        := $(GCOVDIR)/$(HOST_LIBDIR)
GCOV_HOST_GCCFLAGS      := $(HOST_GCCFLAGS) -fprofile-arcs -ftest-coverage
GCOV_HOST_GPPFLAGS      := $(HOST_GPPFLAGS) -fprofile-arcs -ftest-coverage
GCOV_HOST_LDFLAGS       := $(HOST_LDFLAGS) -fprofile-arcs -ftest-coverage

# Most of the needed headers are located here.
INCLUDES := include

# Start out with an empty header file dependency list.  Add module files as we go.
DEPS :=

# Useful macros.
objs = $(addprefix $2/,$(addsuffix .o,$(basename $(wildcard $1/*.c $1/*.cpp $1/*.S))))
armv6m_objs = $(call objs,$1,$(ARMV6M_OBJDIR))
armv7m_objs = $(call objs,$1,$(ARMV7M_OBJDIR))
host_objs = $(call objs,$1,$(HOST_OBJDIR))
gcov_host_objs = $(call objs,$1,$(GCOV_HOST_OBJDIR))
add_deps = $(patsubst %.o,%.d,$(ARMV6M_$1_OBJ) $(ARMV7M_$1_OBJ) $(HOST_$1_OBJ) $(GCOV_HOST_$1_OBJ))
obj_to_gcda = $(patsubst %.o,%.gcda,$1)
includes = $(patsubst %,-I%,$1)
define build_lib
	@echo Building $@
	$Q $(MAKEDIR)
	$Q $($1_AR) -rc $@ $?
endef
define link_exe
	@echo Building $@
	$Q $(MAKEDIR)
	$Q $($1_LD) $($1_LDFLAGS) $^ -o $@
endef
define gcov_link_exe
	@echo Building $@
	$Q $(MAKEDIR)
	$Q $($1_LD) $(GCOV_$1_LDFLAGS) $^ -o $@
endef
define run_gcov
    .PHONY : GCOV_$1
    GCOV_$1 : GCOV_RUN_$1_TESTS
		$Q $(REMOVE) $1_output.txt $(QUIET)
		$Q mkdir -p gcov/$1_tests $(QUIET)
		$Q $(foreach i,$(GCOV_HOST_$1_OBJ),gcov -object-directory=$(dir $i) $(notdir $i) >> $1_output.txt ;)
		$Q mv $1_output.txt gcov/$1_tests/ $(QUIET)
		$Q mv *.gcov gcov/$1_tests/ $(QUIET)
		$Q CppUTest/scripts/filterGcov.sh gcov/$1_tests/$1_output.txt /dev/null gcov/$1_tests/$1.txt
		$Q cat gcov/$1_tests/$1.txt
endef
define make_library # ,LIBRARY,src_dirs,libname.a,includes
    HOST_$1_OBJ      := $(foreach i,$2,$(call host_objs,$i))
    GCOV_HOST_$1_OBJ := $(foreach i,$2,$(call gcov_host_objs,$i))
    HOST_$1_LIB      := $(HOST_LIBDIR)/$3
    GCOV_HOST_$1_LIB := $(GCOV_HOST_LIBDIR)/$3
    DEPS             += $$(call add_deps,$1)
    $$(HOST_$1_LIB)      : INCLUDES := $4
    $$(GCOV_HOST_$1_LIB) : INCLUDES := $4
    $$(HOST_$1_LIB) : $$(HOST_$1_OBJ)
		$$(call build_lib,HOST)
    $$(GCOV_HOST_$1_LIB) : $$(GCOV_HOST_$1_OBJ)
		$$(call build_lib,HOST)
endef
define make_tests # ,LIB2TEST,test_src_dirs,includes,other_libs
    HOST_$1_TESTS_OBJ      := $(foreach i,$2,$(call host_objs,$i))
    GCOV_HOST_$1_TESTS_OBJ := $(foreach i,$2,$(call gcov_host_objs,$i))
    HOST_$1_TESTS_EXE      := $1_tests
    GCOV_HOST_$1_TESTS_EXE := $1_tests_gcov
    DEPS                   += $$(call add_deps,$1_TESTS)
    $$(HOST_$1_TESTS_EXE)      : INCLUDES := CppUTest/include $3
    $$(GCOV_HOST_$1_TESTS_EXE) : INCLUDES := CppUTest/include $3
    $$(HOST_$1_TESTS_EXE) : $$(HOST_$1_TESTS_OBJ) $(HOST_$1_LIB) $(HOST_CPPUTEST_LIB) $4
		$$(call link_exe,HOST)
    .PHONY : RUN_$1_TESTS GCOV_RUN_$1_TESTS
    RUN_$1_TESTS : $$(HOST_$1_TESTS_EXE)
		@echo Runnning $$^
		$Q $$^
    $$(GCOV_HOST_$1_TESTS_EXE) : $$(GCOV_HOST_$1_TESTS_OBJ) $(GCOV_HOST_$1_LIB) $(GCOV_HOST_CPPUTEST_LIB) $4
		$$(call gcov_link_exe,HOST)
    GCOV_RUN_$1_TESTS : $$(GCOV_HOST_$1_TESTS_EXE)
		@echo Runnning $$^
		$Q $$^
endef


# Build CppUTest library which runs on host machine.
$(eval $(call make_library,CPPUTEST,CppUTest/src/CppUTest CppUTest/src/Platforms/Gcc,libCppUTest.a,CppUTest/include))
$(eval $(call make_tests,CPPUTEST,CppUTest/tests,,))

# CrashCatcher core sources to build and test.
ARMV6M_CORE_OBJ    := $(call armv6m_objs,Core)
ARMV7M_CORE_OBJ    := $(call armv7m_objs,Core)
$(eval $(call make_library,CORE,Core/src,libCrashCatcher.a,include Core/tests))
$(eval $(call make_tests,CORE,Core/tests Core/mocks,include Core/tests Core/mocks Core/src,))
$(eval $(call run_gcov,CORE))

# Sources for newlib and mbed's LocalFileSystem semihosting support.
#ARMV7M_SEMIHOST_OBJ := $(call armv7m_objs,semihost)
#ARMV7M_SEMIHOST_OBJ += $(call armv7m_objs,semihost/newlib)
#ARMV7M_SEMIHOST_OBJ += $(call armv7m_objs,semihost/mbed)
#DEPS += $(call add_deps,SEMIHOST)

# Sources for ARMv7-M debug architecture.
#ARM_ARMV7M_OBJ := $(call armv7m_objs,architectures/armv7-m)
#DEPS += $(call add_deps,ARMV7M)

# Native memory access sources.
#ARMV7M_NATIVE_MEM_OBJ := $(call armv7m_objs,memory/native)
#DEPS += $(call add_deps,NATIVE_MEM)

# LPC176x device sources.
#ARMV7M_LPC176X_OBJ := $(call armv7m_objs,devices/lpc176x)
#DEPS += $(call add_deps,LPC176X)


# mbed 1768 board
#ARMV7M_MBED1768_OBJ := $(call armv7m_objs,boards/mbed1768)
#ARMV7M_MBED1768_LIB = $(ARMV7M_LIBDIR)/libmri_mbed1768.a
#$(ARMV7M_MBED1768_LIB) : INCLUDES := $(INCLUDES) boards/mbed1768 devices/lpc176x architecture/armv7-m cmsis/LPC17xx
#$(ARMV7M_MBED1768_LIB) : $(ARMV7M_CORE_OBJ) $(ARMV7M_SEMIHOST_OBJ) $(ARM_ARMV7M_OBJ) $(ARMV7M_NATIVE_MEM_OBJ) $(ARMV7M_LPC176X_OBJ) $(ARMV7M_MBED1768_OBJ)
#	$(call build_lib,ARMV7M)
#DEPS += $(call add_deps,MBED1768)

# All libraries to be built for ARM target.
#ARM_LIBS : $(ARMV7M_MBED1768_LIB)


# *** Pattern Rules ***
$(ARMV6M_OBJDIR)/%.o : %.c
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(ARM_GCC) -mcpu=cortex-m0 $(ARM_GCCFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(ARMV6M_OBJDIR)/%.o : %.S
	@echo Assembling $<
	$Q $(MAKEDIR)
	$Q $(ARM_AS) -mcpu=cortex-m0 $(ARM_ASFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(ARMV7M_OBJDIR)/%.o : %.c
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(ARM_GCC) -mcpu=cortex-m3 $(ARM_GCCFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(ARMV7M_OBJDIR)/%.o : %.S
	@echo Assembling $<
	$Q $(MAKEDIR)
	$Q $(ARM_AS) -mcpu=cortex-m3 $(ARM_ASFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(HOST_OBJDIR)/%.o : %.c
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(HOST_GCC) $(HOST_GCCFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(HOST_OBJDIR)/%.o : %.cpp
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(HOST_GPP) $(HOST_GPPFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(GCOV_HOST_OBJDIR)/%.o : %.c
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(REMOVE) $(call obj_to_gcda,$@) $(QUIET)
	$Q $(HOST_GCC) $(GCOV_HOST_GCCFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(GCOV_HOST_OBJDIR)/%.o : %.cpp
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(REMOVE) $(call obj_to_gcda,$@) $(QUIET)
	$Q $(HOST_GPP) $(GCOV_HOST_GPPFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@


# *** Pull in header dependencies if not performing a clean build. ***
ifneq "$(findstring clean,$(MAKECMDGOALS))" "clean"
    -include $(DEPS)
endif
