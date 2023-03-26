all: lib

.PHONY: lib test sample

lib: 
	mkdir -p build && cd build && cmake ../ $(ZCC_LIB_CMAKE_DEFINITIONS) && make zc zc_coroutine

test sample: lib
	cd build && make

clean: 
	rm -rf build
	mkdir -p build && cd build && cmake ../ $(ZCC_LIB_CMAKE_DEFINITIONS) && make clean >/dev/null
	rm -rf build
	rm -f libzc.a libzc_coroutine.a
	find -type f -name "*~" -exec rm {} \;
	find -type f -name "gmon.out" -exec rm {} \;
	find -type f -name "tags" -exec rm {} \;

