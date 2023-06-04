main:main.cc
	g++ -std=c++11 -o $@ $^

test:UnitTest.cc
	g++ -std=c++11 -o test ThreadCache.cc

.PHONY: clean
clean:
	rm -f main
