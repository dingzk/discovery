TARGET=../lib/libcjson.a

SRC=cJSON.c

${TARGET} : cJSON.o
	ar rcs $@ $<
cJSON.o:${SRC}
	-@mkdir -p ../lib
	$(CC) ${CFLAGS} -fPIC -o $@ -c $<  ${CPPFLAGS}

test: test.c
	${CC} -o $@ $< -L ../lib -lcjson



.PHONY:clean
clean:
	rm -rf *.o a.out test *.a *.so

