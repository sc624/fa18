CPP = g++ -std=c++11
COMPILERFLAGS = -g -Wall -Wextra -Wno-sign-compare
CSMAOBJ = obj/csma.o
LINKLIBS = -lpthread

.PHONY: all clean

all : obj csma

csma: $(CSMAOBJ)
	$(CPP) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)


clean :
obj/%.o: src/%.cpp
	$(CPP) $(COMPILERFLAGS) -c -o $@ $<
obj:
	mkdir -p obj
