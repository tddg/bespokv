################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CXX_SRCS += \
../mklove/modules/configure.cxx 

CC_SRCS += \
../mklove/modules/configure.cc 

OBJS += \
./mklove/modules/configure.o 

CC_DEPS += \
./mklove/modules/configure.d 

CXX_DEPS += \
./mklove/modules/configure.d 


# Each subdirectory must supply rules for building sources it contributes
mklove/modules/%.o: ../mklove/modules/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

mklove/modules/%.o: ../mklove/modules/%.cxx
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


