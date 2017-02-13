################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/rdaddr.o \
../src/rdcrc32.o \
../src/rdgz.o \
../src/rdkafka.o \
../src/rdkafka_assignor.o \
../src/rdkafka_broker.o \
../src/rdkafka_buf.o \
../src/rdkafka_cgrp.o \
../src/rdkafka_conf.o \
../src/rdkafka_feature.o \
../src/rdkafka_msg.o \
../src/rdkafka_offset.o \
../src/rdkafka_op.o \
../src/rdkafka_partition.o \
../src/rdkafka_pattern.o \
../src/rdkafka_queue.o \
../src/rdkafka_range_assignor.o \
../src/rdkafka_request.o \
../src/rdkafka_roundrobin_assignor.o \
../src/rdkafka_subscription.o \
../src/rdkafka_timer.o \
../src/rdkafka_topic.o \
../src/rdkafka_transport.o \
../src/rdlist.o \
../src/rdlog.o \
../src/rdrand.o \
../src/rdstring.o \
../src/snappy.o \
../src/tinycthread.o \
../src/xxhash.o 

C_SRCS += \
../src/rdaddr.c \
../src/rdcrc32.c \
../src/rdgz.c \
../src/rdkafka.c \
../src/rdkafka_assignor.c \
../src/rdkafka_broker.c \
../src/rdkafka_buf.c \
../src/rdkafka_cgrp.c \
../src/rdkafka_conf.c \
../src/rdkafka_feature.c \
../src/rdkafka_msg.c \
../src/rdkafka_offset.c \
../src/rdkafka_op.c \
../src/rdkafka_partition.c \
../src/rdkafka_pattern.c \
../src/rdkafka_queue.c \
../src/rdkafka_range_assignor.c \
../src/rdkafka_request.c \
../src/rdkafka_roundrobin_assignor.c \
../src/rdkafka_sasl.c \
../src/rdkafka_subscription.c \
../src/rdkafka_timer.c \
../src/rdkafka_topic.c \
../src/rdkafka_transport.c \
../src/rdlist.c \
../src/rdlog.c \
../src/rdrand.c \
../src/rdstring.c \
../src/snappy.c \
../src/tinycthread.c \
../src/xxhash.c 

OBJS += \
./src/rdaddr.o \
./src/rdcrc32.o \
./src/rdgz.o \
./src/rdkafka.o \
./src/rdkafka_assignor.o \
./src/rdkafka_broker.o \
./src/rdkafka_buf.o \
./src/rdkafka_cgrp.o \
./src/rdkafka_conf.o \
./src/rdkafka_feature.o \
./src/rdkafka_msg.o \
./src/rdkafka_offset.o \
./src/rdkafka_op.o \
./src/rdkafka_partition.o \
./src/rdkafka_pattern.o \
./src/rdkafka_queue.o \
./src/rdkafka_range_assignor.o \
./src/rdkafka_request.o \
./src/rdkafka_roundrobin_assignor.o \
./src/rdkafka_sasl.o \
./src/rdkafka_subscription.o \
./src/rdkafka_timer.o \
./src/rdkafka_topic.o \
./src/rdkafka_transport.o \
./src/rdlist.o \
./src/rdlog.o \
./src/rdrand.o \
./src/rdstring.o \
./src/snappy.o \
./src/tinycthread.o \
./src/xxhash.o 

C_DEPS += \
./src/rdaddr.d \
./src/rdcrc32.d \
./src/rdgz.d \
./src/rdkafka.d \
./src/rdkafka_assignor.d \
./src/rdkafka_broker.d \
./src/rdkafka_buf.d \
./src/rdkafka_cgrp.d \
./src/rdkafka_conf.d \
./src/rdkafka_feature.d \
./src/rdkafka_msg.d \
./src/rdkafka_offset.d \
./src/rdkafka_op.d \
./src/rdkafka_partition.d \
./src/rdkafka_pattern.d \
./src/rdkafka_queue.d \
./src/rdkafka_range_assignor.d \
./src/rdkafka_request.d \
./src/rdkafka_roundrobin_assignor.d \
./src/rdkafka_sasl.d \
./src/rdkafka_subscription.d \
./src/rdkafka_timer.d \
./src/rdkafka_topic.d \
./src/rdkafka_transport.d \
./src/rdlist.d \
./src/rdlog.d \
./src/rdrand.d \
./src/rdstring.d \
./src/snappy.d \
./src/tinycthread.d \
./src/xxhash.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


