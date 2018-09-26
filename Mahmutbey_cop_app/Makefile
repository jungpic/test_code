##

TARGET = cop__app

all: $(TARGET)

$(TARGET) : $(OBJS)
	rm -rf $(TARGET)
	#cp build-mahmutbey_cop_app_release-Qt_ARM-Debug/cop__app ./
	#cp build-mahmutbey_cop_app_release-cop-Debug/cop__app ./
	cp build-mahmutbey_cop_app_release-cop-Release/cop__app ./
	tar zcf $(TARGET).tar.gz $(TARGET)
	cp $(TARGET).tar.gz /media/jhjo
clean:
	rm -rf $(TARGET)
	rm -rf $(TARGET).tar.gz

cleanall:
	rm -f $(TARGET)
	rm -f $(TARGET).tar.gz

