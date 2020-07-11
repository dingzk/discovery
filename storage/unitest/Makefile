TARGET=test

CFLAGS := -shared -fPIC  -g
INCLUDE := -I ./ -I ../ -I ../../
LDFLAGS := -l pthread

SRC := $(wildcard *.c) $(wildcard ../*.c) $(wildcard ../allocator/*.c) $(wildcard ../allocator/alloc/*.c)
OBJS=$(subst .c,.o,${SRC})

${TARGET}: ${OBJS}
	$(CC) -o $@ ${OBJS} ${INCLUDE}  -g  ${LDFLAGS}
${OBJS}:%.o:%.c
	$(CC) -o $@ -c $< ${CFLAGS} ${INCLUDE}
.PHONY: clean ${TARGET}

clean:
	-rm -rf ${OBJS} ${TARGET} *.dSYM *.o a.out core
