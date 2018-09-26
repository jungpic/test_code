##

TARGET = cop__app

all: $(TARGET)

$(TARGET) : $(OBJS)
	rm -rf $(TARGET)
	cp build-mahmutbey_cop_app_release-Qt_ARM-Debug/cop__app ./
	tar zcf $(TARGET).tar.gz $(TARGET)

clean:
	rm -rf $(TARGET)
	rm -rf $(TARGET).tar.gz

cleanall:
	rm -f $(TARGET)
	rm -f $(TARGET).tar.gz

