# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf-lib/components/ds1307
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/ds1307 -lds1307
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += ds1307
COMPONENT_LDFRAGMENTS += 
component-ds1307-build: component-i2cdev-build component-log-build
