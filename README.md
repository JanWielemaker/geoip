# GeoIP lookup for SWI-Prolog

This library provides a minimal interface to the public GeoIP library
from
[MAXMIND](https://www.maxmind.com/en/geoip2-services-and-databases)

It has been tested with the Ubuntu package `libgeoip-dev` with the
databases from the package `geoip-database-contrib` as well as with
Fedora package `GeoIP-devel`, `GeoIP-GeoLite-data` and
`GeoIP-GeoLite-data-extra` which are by default installed in
`/usr/share/GeoIP`.

A simple usage is:

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

## Installation

*WARNING* This installation will only work on a machine where libgeoip
can be installed in a globally searched place and where C development
tools are provided.

  1. Install GeoIP library and database.
  
     a. The command for Ubuntu 14.04:

      ```{sh}
      $ sudo apt-get install geoip-database-contrib libgeoip-dev
      ```
      
     b. The command for Feadore 24:
     
     ```{sh}
     $ sudo dnf install GeoIP-devel
     ```

  2. Install the pack

      ```{prolog}
      ?- pack_install(geopip).
      ```

## Troubleshooting

If you get the SWI-Prolog exception [1] or the C error [2] you may be
missing the GeoIP data files.

   ```
   [1] ERROR: source_sink `geoip('GeoIPCity.dat')' does not exist
   [2] Invalid database type GeoIP Country Edition, expected GeoIP City Edition, Rev 1
   ```

Install the missing data files from http://geolite.maxmind.com to path
`/usr/local/share/GeoIP` in the following way:

   ```{sh}
   $ sudo su
   $ cd /usr/local/share/GeoIP
   $ wget -N http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz
   $ gunzip GeoLiteCity.dat.gz
   $ ln -s GeoLiteCity.dat GeoIPCity.dat
   ```
