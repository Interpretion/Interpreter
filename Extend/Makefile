CC = llvm-g++
# -stdlib=libc++ -std=c++11
CFLAGS = -g -O3 -I llvm/include -I llvm/build/include -I ./
LLVMFLAGS = `/usr/local/Cellar/llvm/7.0.0/bin/llvm-config --cxxflags --ldflags --system-libs --libs all`
HEADERS = KaleidoscopeJIT.h
EXTRAS = `-rdynamic`

.PHONY: main

main: extend.cpp ${OBJ}
	${CC} ${CFLAGS} ${LLVMFLAGS} ${EXTRAS} $< -o $@

clean:
	rm -r ${OBJ}

%.o: %.cpp ${HEADERS}
	${CC} ${CFLAGS} ${LLVMFLAGS} -c $< -o $@
