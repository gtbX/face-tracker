CXX = g++

LDLIBS = -lusb -lopencv_core -lopencv_objdetect -lopencv_highgui -lopencv_imgproc -lpthread

all:
	$(CXX) -std=gnu++11 -pthread objectDetection.cpp $(LDLIBS) -o objectDetection

