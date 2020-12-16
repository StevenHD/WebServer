CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cpp  List_Timer.cpp locker.cpp task.cpp  WebServer.cpp threadpool.cpp
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread

clean:
	rm  -r server