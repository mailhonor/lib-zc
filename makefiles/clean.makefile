all: clean

clean:
	rm -f libzc.a libzc_coroutine.a
	rm -f tags gmon.out
	find -type f -name "*.o" -exec rm  {} \;
	find -type f -name "*.depend" -exec rm  {} \;
	find -type f -name "*~" -exec rm  {} \;
	find -type f -name "gmon.out" -exec rm  {} \;
	rm -rf OBJS_DEST/
