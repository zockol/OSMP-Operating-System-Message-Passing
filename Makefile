CC=gcc
CFLAGS= -Wall -Wconversion -g 
LIBS=-lm -lpthread
DEPS=

BUILD_DIR := build

SRCS := demo.c 
OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SRCS)))

$(BUILD_DIR)/%.o: %.c $(DEPS)
	mkdir -p $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@


demo: $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS)  $< $(LIBS) -o $@

.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*.o *~ core
	rmdir -p $(BUILD_DIR)

test:
	./demo .

