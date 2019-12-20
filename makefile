#compiler:
CXX = gcc

#flags:
CXXFLAGS=-g -Wall

#objects:
OBJECTS=doSftp.o

#dependencies
DEPENDS=doSftp.h


all: doSftp

doSftp: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lssh

%.o: %.cc $(DEPENDS)
	$(CXX) $(CXXFLAGS) -c $<
	

clean:
	$(RM) doSftp *.o
