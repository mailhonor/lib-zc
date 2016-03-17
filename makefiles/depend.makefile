all: OBJS_DEST/depend

SRCS=${shell find src -type f -name "*.c"}

DEPENDS = $(patsubst %.c, OBJS_DEST/%.depend, $(SRCS))

OBJS_DEST/%.depend: %.c
	@echo -n OBJS_DEST/ > $@
	@dirname $< | tr -d "\n" >> $@
	@echo -n / >> $@
	@$(CC) -E -MM $< -I./ >> $@
	@echo depend $<

OBJS_DEST/depend: $(DEPENDS)
	@cat $(DEPENDS) > OBJS_DEST/depend

