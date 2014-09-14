all:	geoip4pl.so

geoip4pl.so: geoip4pl.c
	swipl-ld -Wall -shared -o geoip4pl geoip4pl.c -lGeoIP

clean:
	rm -f geoip4pl.so
