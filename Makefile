weather_station.bin: project.properties main.ino lib/*
	particle compile photon . --saveTo weather_station.bin

flash: weather_station.bin
	particle flash WeatherStation weather_station.bin

clean:
	rm -f weather_station.bin
