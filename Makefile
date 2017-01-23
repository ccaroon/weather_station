weather_station.bin: main.ino lib/*
	mkdir dist
	cp project.properties main.ino lib/* dist/
	particle compile photon dist/ --saveTo weather_station.bin
	rm -rf dist/

flash: weather_station.bin
	particle flash WeatherStation weather_station.bin

clean:
	rm -f weather_station.bin dist/
