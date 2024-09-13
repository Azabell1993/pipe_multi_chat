# Makefile for Chat Server and Client

CC = gcc
LDFLAGS = -pthread
TARGET_SERVER = chat_server
TARGET_CLIENT = chat_client

SRCS_SERVER = server.c
SRCS_CLIENT = client.c
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-implicit-function-declaration -pthread -Ilib/include

# Default rule
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Server build
$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJS_SERVER) $(LDFLAGS)

# Client build
$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJS_CLIENT) $(LDFLAGS)

# Object file creation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)

# Run server
run_server:
	./$(TARGET_SERVER)

# Run client
run_client:
	./$(TARGET_CLIENT)

.PHONY: all clean run_server run_client
