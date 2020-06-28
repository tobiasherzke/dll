include /usr/share/openmha/config.mk
CXX=g++$(GCC_VER)
CXXFLAGS += -I/usr/include/openmha -Igoogletest/include
LDLIBS += -lopenmha -pthread
plugins: dll.so metronome.so
dll.so: dll.o
	$(CXX) -shared -o $@ $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS)
metronome.so: metronome.o
	$(CXX) -shared -o $@ $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS)
dll.o: dll.cpp dll.hh
dll_unit_tests.o: dll_unit_tests.cpp dll.hh googletest/include/gmock/gmock.h
unit-tests: unit-test-runner
	./unit-test-runner
GTESTLIBS = $(patsubst %, googletest/lib/lib%.a, gmock_main gmock gtest)
unit-test-runner:  dll_unit_tests.o dll.o $(GTESTLIBS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
googletest/include/gmock/gmock.h googletest/lib/libgmock_main.a: googletest/build/Makefile
	$(MAKE) -C googletest/build VERBOSE=1 install

googletest/build/Makefile: googletest/CMakeLists.txt
	mkdir -p googletest/build
	cd googletest/build && cmake -DCMAKE_INSTALL_PREFIX=.. ..

googletest/CMakeLists.txt:
	git clone https://github.com/google/googletest

clean:
	rm -f dll.o dll.so dll_unit_tests.o unit-test-runner
