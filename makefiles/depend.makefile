all: OBJS_DEST/depend

C_SRCS=${shell find src -type f -name "*.c"}
C_DEPENDS = $(patsubst %.c, OBJS_DEST/%.c.depend, $(C_SRCS))

CPP_SRCS=${shell find src -type f -name "*.cpp"}
CPP_DEPENDS = $(patsubst %.cpp, OBJS_DEST/%.cpp.depend, $(CPP_SRCS))

$(C_DEPENDS):OBJS_DEST/%.c.depend: %.c
	@echo -n OBJS_DEST/ > $@
	@dirname $< | tr -d "\n" >> $@
	@echo -n / >> $@
	@gcc -E -MM $< -I./ >> $@
	@echo depend $<

$(CPP_DEPENDS):OBJS_DEST/%.cpp.depend: %.cpp
	@echo -n OBJS_DEST/ > $@
	@dirname $< | tr -d "\n" >> $@
	@echo -n / >> $@
	@g++ -E -MM $< -I./ >> $@
	@echo depend $<

OBJS_DEST/depend: $(C_DEPENDS) $(CPP_DEPENDS)
	@cat $(C_DEPENDS)  $(CPP_DEPENDS) > OBJS_DEST/depend

