# Keep the production library as the default regardless of include order.
.DEFAULT_GOAL := all

include make/config.mk
include make/library.mk
include make/tests.mk

.PHONY: clean
clean:
	rm -rf $(OUT)
