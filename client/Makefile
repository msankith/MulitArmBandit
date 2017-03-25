
CC = g++ -std=c++11
COMPILE = $(CC)
#$(INCLUDES) ${FLAGS} $(LIBS)

SRC_DIR = .
DST_DIR = .

#Target
TARGET = bandit-agent

#Flags
OSFLAGS = -Dlinux
CFLAGS = -O3 -Wall
FLAGS = ${OSFLAGS} ${CFLAGS}

#Sources
SRCS =	bandit-agent.cpp

#Objects
OBJS = $(SRCS:.cpp=.o)

#Rules 	

all:: 	$(TARGET) 

.cpp.o:
	$(COMPILE) -c -o $@ $(@F:%.o=%.cpp)

bandit-agent: $(OBJS)
	$(COMPILE) -o $@ $(OBJS)


clean:
	rm -f $(TARGET) $(OBJS) *~

