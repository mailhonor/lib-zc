all: lib

.PHONY: lib test sample

# export CMAKE_C_COMPILER=/usr/local/bin/gcc-12
# export CMAKE_CXX_COMPILER=/usr/local/bin/g++-12

lib: 
	mkdir -p tmp_build && cd tmp_build && cmake ../ $(ZCC_LIB_CMAKE_DEFINITIONS) && make zcc -j 8

test sample: lib
	cd tmp_build && make

clean: 
	rm -rf build
	rm -rf tmp_build
	mkdir -p tmp_build && cd tmp_build && cmake ../ $(ZCC_LIB_CMAKE_DEFINITIONS) && make clean >/dev/null
	rm -rf tmp_build
	rm -f libzc.a libzcc.a libzc_coroutine.a
	find ./ -type f -name "*~" -exec rm {} \;
	find ./ -type f -name "gmon.out" -exec rm {} \;
	find ./ -type f -name "tags" -exec rm {} \;

