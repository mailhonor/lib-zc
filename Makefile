
all: libzc

.PHONY: test sample

DIRS=${shell find src -type d}
DIRS_DEST = $(patsubst %, OBJS_DEST/%, $(DIRS))
${shell mkdir -p $(DIRS_DEST)}
${shell touch makefiles/defined.include }

libzc lib: depend
	make -f makefiles/lib.makefile

test sample: libzc
	make -f makefiles/sample_list.makefile

depend:
	make -f makefiles/depend.makefile

tag tags:
	ctags -R src/ zc.h

clean:
	make -f makefiles/clean.makefile
	make clean -f makefiles/sample_list.makefile

CLEAN: clean
	rm -r tags

indent:
	find src -name "*.[ch]" -exec Indent {} \;
	find sample -name "*.[ch]" -exec Indent {} \;
	Indent zc.h
