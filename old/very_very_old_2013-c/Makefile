all:
	@echo 
	@echo ".............................. NOW BUILD LIBZYC.SO .........................................."
	cd src; make
	@echo 
	@echo ".............................. NOW BUILD ALL TEST ............................................"
	cd test; make


clean:
	@echo 
	@echo ".............................. NOW CLEAN TMP FILES ............................................"
	find -name "*~" -exec rm {} \;
	find -name "*.o" -exec rm {} \;
	cd src ; make clean
	cd test; make clean


install:
	@echo 
	@echo ".............................. NOW INSTALL LIBZYC.SO V1........................................"
	gcc -Os -Wall -fPIC -shared -DZYC_INUSE $(wildcard src/*.c) -o libzyc.so
	strip libzyc.so
	mv libzyc.so /usr/local/lib/libzyc1.so

