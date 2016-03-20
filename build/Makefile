# Build file for FX3 sample applications

TARGET_DIR:=$(dir $(wildcard */Makefile))

all: $(TARGET_DIR)

clean: $(TARGET_DIR)

$(TARGET_DIR): force
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all force
