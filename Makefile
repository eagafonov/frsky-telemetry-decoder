all:

test: build-test
	test/all_test

build-test:
	g++ -I`pwd`/../ -o test/all_test test/main.cpp test/test_*.cpp decoder.cpp -l cppunit

.PHONY: test
