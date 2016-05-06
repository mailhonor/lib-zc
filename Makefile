
all: libwow sample_part

.PHONY: test sample

DIRS=${shell find src -type d}
DIRS_DEST = $(patsubst %, OBJS_DEST/%, $(DIRS))
${shell mkdir -p $(DIRS_DEST)}

libwow lib wow: depend
	make -f makefiles/lib.makefile

test sample: libwow
	make -f makefiles/sample_list.makefile samples=ALL

sample_part: libwow
	@touch plist
	make -f makefiles/sample_list.makefile samples=PART

depend:
	make -f makefiles/depend.makefile

tag tags:
	ctags -R src/ libzc.h

clean:
	make -f makefiles/clean.makefile
	make clean -f makefiles/sample_list.makefile

CLEAN: clean
	rm -r tags plist

indent:
	find src -name "*.[ch]" -exec Indent {} \;
	find sample -name "*.[ch]" -exec Indent {} \;
	Indent libzc.h
