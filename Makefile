CXX = g++

LDLIBS = -lusb -lopencv_core -lopencv_objdetect -lopencv_highgui -lopencv_imgproc

all:
	$(CXX)  objectDetection.cpp $(LDLIBS) -o objectDetection

