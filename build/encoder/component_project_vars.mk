# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf-lib/components/encoder
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/encoder -lencoder
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += encoder
COMPONENT_LDFRAGMENTS += 
component-encoder-build: component-i2cdev-build component-log-build
