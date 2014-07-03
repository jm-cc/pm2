CC = g++			#.
CFLAGS = -Wall -std=c++11
LDFLAGS =
EXEC_NAME = paje_parser
INCLUDES = -Iinclude/
SOURCES= Color.cpp Entity.cpp Hex.cpp ParserDefinitionPaje.cpp StateType.cpp VariableType.cpp Container.cpp EntityType.cpp Info.cpp Name.cpp ParserEventPaje.cpp Statistic.cpp EntityValue.cpp Integer.cpp PajeDefinition.cpp ParserPaje.cpp String.cpp ContainerType.cpp Event.cpp Interval.cpp  PajeFileManager.cpp Tools.cpp Date.cpp EventL.cpp Link.cpp paje_parser.cpp StateChange.cpp Double.cpp EventType.cpp LinkType.cpp State.cpp Variable.cpp TraceL.cpp
CFLAGS += $(shell pkg-config --cflags nmad)
LDFLAGS += $(shell pkg-config --libs nmad)

OBJ_FILES = $(SOURCES:.cpp=.o) 
INSTALL_DIR = bin/

all : $(SOURCES) $(EXEC_NAME)

clean :
	rm -f *~ *.o ${EXEC_NAME}

$(EXEC_NAME) : $(OBJ_FILES)
	$(CC)  -o $(EXEC_NAME) $(OBJ_FILES) $(LDFLAGS)

%.o: %.cpp
	$(CC)  $(CFLAGS) $(INCLUDES) -o $@ -c $<

%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

%.o: %.c
	gcc $(CFLAGS) $(INCLUDES) -o $@ -c $<

install :
	cp $(EXEC_NAME) $(INSTALL_DIR)

