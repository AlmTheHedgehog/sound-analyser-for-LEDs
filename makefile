# List your *.h files (if you do not have them in your project then leave the variable "headers" empty):
headers = src/SoundAnalyser.hpp src/BluetoothCommunicator.hpp src/CommunicatorI.hpp src/TcpCommunicator.hpp

# List your *.cpp files:
sources = src/SoundAnalyser.cpp src/BluetoothCommunicator.cpp src/TcpCommunicator.cpp src/main.cpp

# List your libs -l* files:
libs = -lfftw3 -lfftw3_omp -lportaudio -fopenmp -lbluetooth

includePath = /home/almthehedgehog/Documents/home/sound-analyser-for-LEDs/include/

libsPath = /home/almthehedgehog/Documents/home/sound-analyser-for-LEDs/include/

# Specify name of your program:
executable = target/SoundAnalyser

$(executable): $(headers) $(sources)
	g++ -g -Wall -pedantic -I$(includePath) -L$(libsPath) $(libs) $(sources) -o $(executable)

.PHONY: clean
clean:
	rm $(executable)

.PHONY: check
check: $(executable)
	valgrind --leak-check=full --track-origins=yes ./$(executable)

.PHONY: run
run:
	./target/SoundAnalyser