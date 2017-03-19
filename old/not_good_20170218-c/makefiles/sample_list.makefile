all: sample
.PHONY: sample

ALL_DIRS=${dir ${shell ls sample/*/Makefile}}

PART_DIRS_TMP=${shell cat plist}
PART_DIRS=$(patsubst %,sample/%,$(PART_DIRS_TMP))

ifeq ($(samples),  ALL)
	DIRS=$(ALL_DIRS)
else
	DIRS=$(PART_DIRS)
endif

sample:
	@set -e; for dir in $(DIRS); do \
		(set -e; cd $$dir; make targetFromTop) || exit 1; \
		done;

clean:
	@echo clean all smaples
	@set -e; for dir in $(ALL_DIRS); do \
		(set -e; cd $$dir;make CLEAN) || exit 1; \
		done;
