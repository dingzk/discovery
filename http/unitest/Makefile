TARGET=test

CFLAGS := -shared -fPIC -std=c++11 -g
INCLUDE := -I ./ -I ../ -I ../../ -I ../../compressor/zlib
LDFLAGS := -l pthread

SRC := $(wildcard *.cpp) $(wildcard ../*.cpp) $(wildcard ../events/*.cpp) $(wildcard ../../common/*.cpp)
ZLIB_SRC = $(wildcard ../../compressor/zlib/*.c)
OBJS=$(subst .cpp,.o,${SRC})
OBJS2=$(subst .c,.o,${ZLIB_SRC})

${TARGET}: ${OBJS} ${OBJS2}
	$(CXX) -o $@ ${OBJS} ${OBJS2} ${INCLUDE} -std=c++11 -g  ${LDFLAGS}

${OBJS}:%.o:%.cpp
	$(CXX) -o $@ -c $< ${CFLAGS} ${INCLUDE}
${OBJS2}:%.o:%.c
	gcc -o $@ -c $< -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN ${INCLUDE}

.PHONY: clean ${TARGET}

clean:
	-rm -rf ${OBJS} ${OBJS2} ${TARGET} *.dSYM *.o a.out core



