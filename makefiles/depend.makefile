all: .depend

-include makefiles/defined.include

GCC_SRCS = ${shell find src cpp_src -type f -name "*.c"}
GCC_DEPENDS = $(patsubst %,%.depend, $(GCC_SRCS))

GPP_SRCS = ${shell find src cpp_src -type f -name "*.cpp"}
GPP_DEPENDS = $(patsubst %,%.depend, $(GPP_SRCS))

$(GCC_DEPENDS):%.depend: %
	@echo depend $<
	@echo -n > $@.tmp
	@dirname $< | tr -d "\n" >> $@.tmp
	@echo -n / >> $@.tmp
	@gcc -E -MM $< -I./ -D___ZC_DEV_MODE___ $(EXTRA_CFLAGS) >/dev/null || exit 1
	@gcc -E -MM $< -I./ -D___ZC_DEV_MODE___ $(EXTRA_CFLAGS) | sed "s/\.o: /.c.o: /" >> $@.tmp
	@mv $@.tmp $@

$(GPP_DEPENDS):%.depend: %
	@echo depend $<
	@echo -n > $@.tmp
	@dirname $< | tr -d "\n" >> $@.tmp
	@echo -n / >> $@.tmp
	@gcc -E -MM $< -I./ -D___ZC_DEV_MODE___ $(EXTRA_CFLAGS) >/dev/null || exit 1
	@gcc -E -MM $< -I./ -D___ZC_DEV_MODE___ $(EXTRA_CFLAGS) | sed "s/\.o: /.cpp.o: /" >> $@.tmp
	@mv $@.tmp $@

.depend: $(GCC_DEPENDS) $(GPP_DEPENDS)
	@cat $(GCC_DEPENDS)  $(GPP_DEPENDS) > .depend

