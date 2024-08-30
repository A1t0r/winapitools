SOURCES = winapitools.cpp
DLL = $(SOURCES:.cpp=.dll)
CXX=g++.exe
CXXFLAGS=-std=c++23 -Wall
OBJS = winapitools.o

.PHONY: clean rebuild

$(DLL): $(OBJS)	
	$(CXX) $(OBJS) -shared -o $(DLL)

$(OBJS): $(SOURCES)
	$(CXX) -c $(SOURCES)

rebuild: clean $(SOURCES) $(DLL)

clean:
	del *.o
	del *.dll