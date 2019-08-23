# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf-lib/components/dht
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/dht -ldht
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += dht
COMPONENT_LDFRAGMENTS += 
component-dht-build: component-driver-build component-esp32-build component-freertos-build
