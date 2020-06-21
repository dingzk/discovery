TARGET=main

CFLAGS := -shared -fPIC -std=c++11 -g
INCLUDE := -I ./ -I ../
LDFLAGS := -l pthread

SRC := $(wildcard ./*.cpp) $(wildcard ./events/*.cpp) $(wildcard ../common/*.cpp)
OBJS=$(subst .cpp,.o,${SRC})

${TARGET}: ${OBJS}
	$(CXX) -o $@ ${OBJS} ${INCLUDE} -std=c++11 -g  ${LDFLAGS}

${OBJS}:%.o:%.cpp
	$(CXX) -o $@ -c $< ${CFLAGS} ${INCLUDE}



.PHONY: clean ${TARGET}

clean:
	-rm -rf ${OBJS} ${TARGET} *.dSYM a.out core



