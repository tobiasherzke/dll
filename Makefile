include /usr/share/openmha/config.mk
CXX=gcc$(GCC_VER)
CXXFLAGS += -I/usr/include/openmha
dll.so: dll.o
dll.o: dll.cpp dll.hh

