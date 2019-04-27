.PHONY: all, clean
TARGET = pi
CSRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(CSRCS))
CC = gcc
HDRS = $(wildcard *.h)
all: $(TARGET)
$(TARGET) : $(OBJS)
	$(CC) -pthread -o $(TARGET) $(OBJS) 
%.o: %.c $(HDRS)
	@echo "$< | $? | $*"
	$(CC) -c $< -o $@ 
clean:
	rm -R $(TARGET)
	rm -R $(OBJS)  
