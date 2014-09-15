# GeoIP lookup for SWI-Prolog

This library provides a minimal interface   to  the public GeoIP library
from [MAXMIND](https://www.maxmind.com/en/geoip2-services-and-databases)

It has been tested  with  the   Ubuntu  package  =libgeoip-dev= with the
databases  from  the  package  =geoip-database-contrib=,  which  are  by
default installed in =/usr/share/GeoIP=.  A simple usage is:

  ```{prolog}
  ?- tcp_host_to_address('www.swi-prolog.org', IP),
     geoip_lookup(IP, R).
  R = geoip{city:'Amsterdam',
	    continent_code:'EU',
	    country_code:'NL',
	    country_code3:'NLD',
	    country_name:'Netherlands',
	    latitude:52.349998474121094,
	    longitude:4.9166998863220215,
	    netmask:16,
	    region:'07'}.
  ```