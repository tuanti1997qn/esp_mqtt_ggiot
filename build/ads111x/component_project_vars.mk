# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf-lib/components/ads111x
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/ads111x -lads111x
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += ads111x
COMPONENT_LDFRAGMENTS += 
component-ads111x-build: component-i2cdev-build component-log-build
