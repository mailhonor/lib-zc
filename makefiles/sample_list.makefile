all: sample
.PHONY: sample

DIRS=${dir ${shell ls sample/*/Makefile cpp_sample/*/Makefile}}

sample:
	@set -e; for dir in $(DIRS); do \
		(set -e; cd $$dir; make targetFromTop) || exit 1; \
		done;

clean:
	@set -e; for dir in $(DIRS); do \
		(set -e; cd $$dir;make cleanFromTop) || exit 1; \
		done;

