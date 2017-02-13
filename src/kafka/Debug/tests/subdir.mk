################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../tests/xxxx-metadata.cpp 

C_SRCS += \
../tests/0001-multiobj.c \
../tests/0002-unkpart.c \
../tests/0003-msgmaxsize.c \
../tests/0004-conf.c \
../tests/0005-order.c \
../tests/0006-symbols.c \
../tests/0007-autotopic.c \
../tests/0008-reqacks.c \
../tests/0011-produce_batch.c \
../tests/0012-produce_consume.c \
../tests/0013-null-msgs.c \
../tests/0014-reconsume-191.c \
../tests/0015-offset_seeks.c \
../tests/0017-compression.c \
../tests/0018-cgrp_term.c \
../tests/0019-list_groups.c \
../tests/0020-destroy_hang.c \
../tests/0021-rkt_destroy.c \
../tests/0022-consume_batch.c \
../tests/0025-timers.c \
../tests/0026-consume_pause.c \
../tests/0028-long_topicnames.c \
../tests/0029-assign_offset.c \
../tests/0030-offset_commit.c \
../tests/0031-get_offsets.c \
../tests/0033-regex_subscribe.c \
../tests/0034-offset_reset.c \
../tests/0035-api_version.c \
../tests/0036-partial_fetch.c \
../tests/0037-destroy_hang_local.c \
../tests/0038-performance.c \
../tests/1000-unktopic.c \
../tests/test.c \
../tests/xxxx-assign_partition.c 

OBJS += \
./tests/0001-multiobj.o \
./tests/0002-unkpart.o \
./tests/0003-msgmaxsize.o \
./tests/0004-conf.o \
./tests/0005-order.o \
./tests/0006-symbols.o \
./tests/0007-autotopic.o \
./tests/0008-reqacks.o \
./tests/0011-produce_batch.o \
./tests/0012-produce_consume.o \
./tests/0013-null-msgs.o \
./tests/0014-reconsume-191.o \
./tests/0015-offset_seeks.o \
./tests/0017-compression.o \
./tests/0018-cgrp_term.o \
./tests/0019-list_groups.o \
./tests/0020-destroy_hang.o \
./tests/0021-rkt_destroy.o \
./tests/0022-consume_batch.o \
./tests/0025-timers.o \
./tests/0026-consume_pause.o \
./tests/0028-long_topicnames.o \
./tests/0029-assign_offset.o \
./tests/0030-offset_commit.o \
./tests/0031-get_offsets.o \
./tests/0033-regex_subscribe.o \
./tests/0034-offset_reset.o \
./tests/0035-api_version.o \
./tests/0036-partial_fetch.o \
./tests/0037-destroy_hang_local.o \
./tests/0038-performance.o \
./tests/1000-unktopic.o \
./tests/test.o \
./tests/xxxx-assign_partition.o \
./tests/xxxx-metadata.o 

C_DEPS += \
./tests/0001-multiobj.d \
./tests/0002-unkpart.d \
./tests/0003-msgmaxsize.d \
./tests/0004-conf.d \
./tests/0005-order.d \
./tests/0006-symbols.d \
./tests/0007-autotopic.d \
./tests/0008-reqacks.d \
./tests/0011-produce_batch.d \
./tests/0012-produce_consume.d \
./tests/0013-null-msgs.d \
./tests/0014-reconsume-191.d \
./tests/0015-offset_seeks.d \
./tests/0017-compression.d \
./tests/0018-cgrp_term.d \
./tests/0019-list_groups.d \
./tests/0020-destroy_hang.d \
./tests/0021-rkt_destroy.d \
./tests/0022-consume_batch.d \
./tests/0025-timers.d \
./tests/0026-consume_pause.d \
./tests/0028-long_topicnames.d \
./tests/0029-assign_offset.d \
./tests/0030-offset_commit.d \
./tests/0031-get_offsets.d \
./tests/0033-regex_subscribe.d \
./tests/0034-offset_reset.d \
./tests/0035-api_version.d \
./tests/0036-partial_fetch.d \
./tests/0037-destroy_hang_local.d \
./tests/0038-performance.d \
./tests/1000-unktopic.d \
./tests/test.d \
./tests/xxxx-assign_partition.d 

CPP_DEPS += \
./tests/xxxx-metadata.d 


# Each subdirectory must supply rules for building sources it contributes
tests/%.o: ../tests/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

tests/%.o: ../tests/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


