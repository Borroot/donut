EXTRAS = -Wno-unused-parameter
CFLAGS = $(EXTRAS) -Wall -Wextra -Werror -pedantic
LFLAGS = -lm

TARGET = donut

all: $(TARGET)

$(TARGET): % : %.c
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET)

re: clean all

.PHONY: all clean re
