include makefile.conf

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(WOPTION) -c $(SRCS)
	$(CC) -o $@ $^ $(LIB_DIR) $(LIBS)

clean:
	$(RM) $(OBJS)
	$(RM) ../src/*.o
	$(RM) $(TARGET)

