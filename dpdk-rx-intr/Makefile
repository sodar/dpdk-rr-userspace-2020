ifneq ($(shell pkg-config --exists libdpdk && echo 0),0)
$(error "libdpdk is not installed")
endif

ifneq ($(shell pkg-config --exists libibverbs && echo 0),0)
$(error "libibverbs is not installed")
endif

APP := dpdk-rx-intr

CFLAGS := \
	-O0 \
	-g \
	-DALLOW_EXPERIMENTAL_API \
	-DRTE_ENABLE_ASSERT \
	$(shell pkg-config --cflags libdpdk) \
	$(null)

LDFLAGS := $(shell pkg-config --static --libs libdpdk)

all: build/$(APP)

build/$(APP): main.c | build
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -rf build
