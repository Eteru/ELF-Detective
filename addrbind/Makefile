CC=g++
CFLAGS=-std=c++11 -c
LDFLAGS=-lbfd -lopcodes
SOURCES=tools.cpp ELFFile.cpp addrbind.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=elfdetective

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) ; $(CC) $(OBJECTS) -o $@ $(LDFLAGS) 

.cpp.o: ; $(CC) $(CFLAGS) $< -o $@

clean: ; rm $(EXECUTABLE) $(OBJECTS)
