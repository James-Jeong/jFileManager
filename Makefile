include makefile.conf

all: $(TARGET)

$(TARGET): $(JFILEMANAGER_OBJS)
	$(AR) $@ $^

clean:
	$(RM) $(jFILEMANAGER_OBJS)
	$(RM) $(TARGET)

