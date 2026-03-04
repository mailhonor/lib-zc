all: lib

.PHONY: lib test sample

# export CMAKE_C_COMPILER=/usr/local/bin/gcc-12
# export CMAKE_CXX_COMPILER=/usr/local/bin/g++-12

lib: 
	mkdir -p tmp_build && cd tmp_build && cmake ../ $(ZCC_LIB_CMAKE_DEFINITIONS) && \
		make zcc zcc_static zcc_sqlite3 zcc_icu zcc_coroutine_extend zc_static zc_coroutine zc_coroutine_static -j 4

clean: 
	rm -rf build
	rm -rf tmp_build
	mkdir -p tmp_build && cd tmp_build && cmake ../ $(ZCC_LIB_CMAKE_DEFINITIONS) && make clean >/dev/null
	rm -rf tmp_build
	rm -f *.a *.so
	find ./ -type f -name "*~" -exec rm {} \;
	find ./ -type f -name "gmon.out" -exec rm {} \;
	find ./ -type f -name "tags" -exec rm {} \;

