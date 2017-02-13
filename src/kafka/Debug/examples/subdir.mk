################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../examples/kafkatest_verifiable_client.cpp \
../examples/rdkafka_consumer_example.cpp \
../examples/rdkafka_example.cpp 

C_SRCS += \
../examples/rdkafka_consumer_example.c \
../examples/rdkafka_example.c \
../examples/rdkafka_performance.c \
../examples/rdkafka_zookeeper_example.c 

OBJS += \
./examples/kafkatest_verifiable_client.o \
./examples/rdkafka_consumer_example.o \
./examples/rdkafka_example.o \
./examples/rdkafka_performance.o \
./examples/rdkafka_zookeeper_example.o 

C_DEPS += \
./examples/rdkafka_consumer_example.d \
./examples/rdkafka_example.d \
./examples/rdkafka_performance.d \
./examples/rdkafka_zookeeper_example.d 

CPP_DEPS += \
./examples/kafkatest_verifiable_client.d \
./examples/rdkafka_consumer_example.d \
./examples/rdkafka_example.d 


# Each subdirectory must supply rules for building sources it contributes
examples/%.o: ../examples/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

examples/%.o: ../examples/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


