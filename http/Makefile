TARGET: test

CFLAGS := -std=c++11 -g
INCLUDE := -I ../common
LDFLAGS := -L pthread

SRC := $(wildcard ./*.cpp) $(wildcard ./events/*.cpp) $(wildcard ../common/*.cpp)
OBJS=$(subst .cpp,.o,${SRC})

${TARGET}: ${OBJS}
	$(CXX) -o $@ ${OBJS} ${CFLAGS} ${INCLUDE}  ${LDFLAGS}

${OBJS}:%.o:%.cpp
	$(CXX) -o $@ -c $< ${CFLAGS} ${INCLUDE}


.PHONY: clean ${TARGET}

clean:
	-rm -rf ${OBJS} ${TARGET} *.dSYM a.out core



