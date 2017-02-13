################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src-cpp/ConfImpl.o \
../src-cpp/ConsumerImpl.o \
../src-cpp/HandleImpl.o \
../src-cpp/KafkaConsumerImpl.o \
../src-cpp/MessageImpl.o \
../src-cpp/MetadataImpl.o \
../src-cpp/ProducerImpl.o \
../src-cpp/QueueImpl.o \
../src-cpp/RdKafka.o \
../src-cpp/TopicImpl.o \
../src-cpp/TopicPartitionImpl.o 

CPP_SRCS += \
../src-cpp/ConfImpl.cpp \
../src-cpp/ConsumerImpl.cpp \
../src-cpp/HandleImpl.cpp \
../src-cpp/KafkaConsumerImpl.cpp \
../src-cpp/MessageImpl.cpp \
../src-cpp/MetadataImpl.cpp \
../src-cpp/ProducerImpl.cpp \
../src-cpp/QueueImpl.cpp \
../src-cpp/RdKafka.cpp \
../src-cpp/TopicImpl.cpp \
../src-cpp/TopicPartitionImpl.cpp 

OBJS += \
./src-cpp/ConfImpl.o \
./src-cpp/ConsumerImpl.o \
./src-cpp/HandleImpl.o \
./src-cpp/KafkaConsumerImpl.o \
./src-cpp/MessageImpl.o \
./src-cpp/MetadataImpl.o \
./src-cpp/ProducerImpl.o \
./src-cpp/QueueImpl.o \
./src-cpp/RdKafka.o \
./src-cpp/TopicImpl.o \
./src-cpp/TopicPartitionImpl.o 

CPP_DEPS += \
./src-cpp/ConfImpl.d \
./src-cpp/ConsumerImpl.d \
./src-cpp/HandleImpl.d \
./src-cpp/KafkaConsumerImpl.d \
./src-cpp/MessageImpl.d \
./src-cpp/MetadataImpl.d \
./src-cpp/ProducerImpl.d \
./src-cpp/QueueImpl.d \
./src-cpp/RdKafka.d \
./src-cpp/TopicImpl.d \
./src-cpp/TopicPartitionImpl.d 


# Each subdirectory must supply rules for building sources it contributes
src-cpp/%.o: ../src-cpp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


