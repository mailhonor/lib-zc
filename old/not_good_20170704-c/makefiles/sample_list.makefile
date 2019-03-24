all: sample
.PHONY: sample

DIRS=${dir ${shell ls sample/*/Makefile}}

sample:
	@set -e; for dir in $(DIRS); do \
		(set -e; cd $$dir; make targetFromTop) || exit 1; \
		done;

clean:
	@echo clean all smaples
	@set -e; for dir in $(DIRS); do \
		(set -e; cd $$dir;make CLEAN) || exit 1; \
		done;
