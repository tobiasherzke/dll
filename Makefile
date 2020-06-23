include /usr/share/openmha/config.mk
CXX=g++$(GCC_VER)
CXXFLAGS += -I/usr/include/openmha -Igoogletest/include
LDLIBS += -lopenmha -pthread
dll.so: dll.o
dll.o: dll.cpp dll.hh

unit-tests: unit-test-runner
	./unit-test-runner
GTESTLIBS = $(patsubst %, googletest/lib/lib%.a, gmock_main gmock gtest)
unit-test-runner:  dll_unit_tests.o dll.o $(GTESTLIBS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
googletest/lib/libgmock_main.a: googletest/build/Makefile
	$(MAKE) -C googletest/build VERBOSE=1 install

googletest/build/Makefile: googletest/CMakeLists.txt
	mkdir -p googletest/build
	cd googletest/build && cmake -DCMAKE_INSTALL_PREFIX=.. ..

googletest/CMakeLists.txt:
	git clone git@github.com:google/googletest

clean:
	rm -f dll.o dll.so dll_unit_tests.o unit-test-runner
