all: clean

clean:
	rm -f libzc.a libzc_coroutine.a
	rm -f .depend
	find src cpp_src -type f -name "*.o" -exec rm  {} \;
	find src cpp_src -type f -name "*.depend" -exec rm  {} \;
	find src cpp_src -type f -name "*.depend.tmp" -exec rm  {} \;
	find -type f -name "*~" -exec rm  {} \;
	find -type f -name "gmon.out" -exec rm  {} \;
	find -type f -name "tags" -exec rm  {} \;

