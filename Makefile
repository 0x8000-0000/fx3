# Build file for FX3 and apps

all:
	$(MAKE) -C source/modules
	$(MAKE) -C build

clean:
	$(MAKE) -C source/modules clean
	$(MAKE) -C build clean

.PHONY: all clean
