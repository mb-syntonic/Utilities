#------------------------------------------------------------------
# Makefile - Make rules for onePK sample applications
#
# Copyright (c) 2011-2013 by Cisco Systems, Inc.
# All rights reserved.
#------------------------------------------------------------------

ifeq ($(origin ONEP_SDK), undefined)
    $(error Please ensure ONEP_SDK points to <sdk location> prior to running make. \
    Example: export ONEP_SDK=/opt/cisco/onep/c32/sdk-c32-1.0.0.0)
endif

CC ?= gcc

CC_VERSION := $(shell $(CC) -dumpversion)

ifndef CC_VERSION
    $(error The C compiler you are trying to use could not be determined. \
            Please use GCC version 4.5 or later to compile onePK applications)
endif

CC_V_MAJOR := $(shell echo $(CC_VERSION) | awk -F. '{ print $1 }')

ifndef CC_V_MAJOR
    $(error The C compiler you are trying to use could not be determined. \
            Please use GCC version 4.5 or later to compile onePK applications)
endif

CC_V_MINOR := $(shell echo $(CC_VERSION) | awk -F. '{ print $2 }')

ifndef CC_V_MINOR
    $(error The C compiler you are trying to use could not be determined. \
            Please use GCC version 4.5 or later to compile onePK applications)
endif

CC_V_GTE_45 := $(shell expr \( $(CC_V_MAJOR) = 4 \& $(CC_V_MINOR) \>= 5 \) \
                \| $(CC_V_MAJOR) \> 4)

ifneq ($(CC_V_GTE_45), 1)
    $(error The C compiler you are trying to use is too old. Please update to \
            GCC version 4.5 or later to compile onePK applications.)
endif

# --------------------------------------
# Build 32 bit as a default 
# To build 64 bit, update LBITS to 64
# for setting some FLAGS
# --------------------------------------
ONEP_LBITS=32
ifeq ($(LBITS),64)
    # compile 64 bit
    ONEP_LBITS=64
else
    # compile 32 bit
    ONEP_LBITS=32
endif


# ----------------
# Common GCC Variables
# ----------------
CC_BLDFLAGS := \
    -Wall -c -fPIE -O1 -D_FORTIFY_SOURCE=2 -m${ONEP_LBITS} 

CC_INCDIRS := \
    -I${ONEP_SDK}/c/include

LD_LIBDIRS := \
    -L${ONEP_SDK}/c/lib 

LD_LIBS := \
    -lonep${ONEP_LBITS}_core -lssp -lrt 

LD_FLAGS := \
    -m${ONEP_LBITS} -pie

# Objects file to build
OBJSNETAPP += \
    obj/onePKRouterConfigurator.o

# All Target
all: makedirs bin/onePKRouterConfigurator
#all: test

# Add inputs and outputs from these tool invocations to the build variables 
test:
	@echo 'onep_sdk : $(ONEP_SDK)'

makedirs:
	@if [ ! -d obj ] ; then mkdir obj ; fi
	@if [ ! -d bin ] ; then mkdir bin ; fi

# Tool invocations
bin/onePKRouterConfigurator: makedirs $(OBJSNETAPP)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker $(LD_LIBDIRS) $(LD_LIBS) $(LD_FLAGS)'
	$(CC) -o $@ \
        $(OBJSNETAPP) $(LD_FLAGS) $(LD_LIBDIRS) $(LD_LIBS) 
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) -rf obj bin 
	-@echo ' '

obj/%.o: %.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) $(CC_INCDIRS) $(CC_BLDFLAGS) -MMD -MP \
		-MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

.PHONY: all clean dependents
.SECONDARY:
