# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf-lib/components/bmp280
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/bmp280 -lbmp280
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += bmp280
COMPONENT_LDFRAGMENTS += 
component-bmp280-build: component-i2cdev-build component-log-build
