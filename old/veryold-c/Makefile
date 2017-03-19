all: tags libzc-debug tlist

.PHONY: test tlist

libzc-debug: tags
	make -f makefile.libzc.debug

libzc zclib:
	make -f makefile.libzc

test: tags libzc-debug
	make -f makefile.test

tlist: tags libzc-debug
	touch tlist
	make -f makefile.tlist

install: libzc
	cp -a libzc.a /usr/local/lib/libzc.a
	cp -a zc.h /usr/local/include/libzc.h

indent:
	Indent zc.h
	Indent src/zc_inuse.h
	find src -type f -name "*.c" -exec Indent {} \;
	find test -type f -name "*.c" -exec Indent {} \;
	find -type f -name "*~" -exec rm  {} \;
	ctags -R zc.h src test

tags:
	ctags -R zc.h src test

clean:
	rm -f libzc.a libzc-debug.a
	rm -f tags gmon.out
	find -type f -name "*.o" -exec rm  {} \;
	find -type f -name "*.r" -exec rm  {} \;
	find -type f -name "*~" -exec rm  {} \;
	make -f makefile.test clean
	ctags -R zc.h src test
