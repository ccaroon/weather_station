weather_station.bin: main.ino lib/*.h lib/*.cpp
	particle compile photon . --saveTo weather_station.bin

flash: weather_station.bin
	particle flash WeatherStation weather_station.bin

clean:
	rm -f weather_station.bin
