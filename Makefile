include /usr/share/openmha/config.mk
CXX=g++$(GCC_VER)
CXXFLAGS += -I/usr/include/openmha -Igoogletest/include -fPIC
LDLIBS += -lopenmha -pthread
plugins: dll.so metronome.so wav2lsl.so lsl2wav.so timestamper.so
%.so: %.o
	$(CXX) -shared -fPIC -o $@ $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS)
wav2lsl.so lsl2wav.so: LDLIBS += -llsl
dll.o: dll.cpp dll.hh
timestamper.o: timestamper.cpp timestamper.hh
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
	rm -f *.so *.o unit-test-runner
