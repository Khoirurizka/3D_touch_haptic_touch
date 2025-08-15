CC = gcc
CFLAGS += -W -g -DNDEBUG -Dlinux
CFLAGS += -I/usr/include/QH/ \
          -I/opt/OpenHaptics/include   # HD/HDU headers path

LIBS = -lHD -lHDU -lpthread -lstdc++ -lm

TARGET = read_force
SRCS = read_force.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(TARGET)

