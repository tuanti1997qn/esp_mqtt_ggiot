# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf-lib/components/ultrasonic
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/ultrasonic -lultrasonic
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += ultrasonic
COMPONENT_LDFRAGMENTS += 
component-ultrasonic-build: component-driver-build component-esp32-build component-freertos-build
