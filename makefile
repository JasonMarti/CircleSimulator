circleSim : circleMain.o
	g++ -Wall -o circleSim circleMain.o -pthread
circleMain.o : circleMain.cpp
	g++ -Wall -c circleMain.cpp -pthread