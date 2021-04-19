
all: libzc

.PHONY: test sample

libzc lib: depend
	make -f makefiles/lib.makefile $(libzc_special_target)

test sample: libzc
	make -f makefiles/sample_list.makefile

depend:
	make -f makefiles/depend.makefile

tag tags:
	ctags -R src/ zc.h

clean:
	make -f makefiles/clean.makefile
	make clean -f makefiles/sample_list.makefile
	@echo -e "\nclean over \n"

CLEAN: clean
	rm -r tags

indent:
	find src -name "*.[ch]" -exec Indent {} \;
	find sample -name "*.[ch]" -exec Indent {} \;
	Indent zc.h
