/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (C): 2014, VU University, Amsterdam

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, if you link this library with other files,
    compiled with a Free Software compiler, to produce an executable, this
    library does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

:- module(geoip,
	  [ geoip_open/2,			% +Name, -Handle
	    geoip_open/3,			% +File, +Mode, -Handle
	    geoip_close/1,			% +Handle
	    geoip_close/0,
	    geoip_lookup/2,			% +From, -Result
	    geoip_lookup/3			% +Handle, +From, -Result
	  ]).
:- use_module(library(apply)).
:- use_foreign_library(foreign(geoip4pl)).

:- multifile user:file_search_path/2.

user:file_search_path(geoip, '/usr/share/GeoIP').

/** <module> GeoIP lookup

This library provides a minimal interface   to  the public GeoIP library
from [MAXMIND](https://www.maxmind.com/en/geoip2-services-and-databases)

It has been tested  with  the   Ubuntu  package  =libgeoip-dev= with the
databases  from  the  package  =geoip-database-contrib=,  which  are  by
default installed in =/usr/share/GeoIP=.  A simple usage is:

  ==
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
  ==

@tbd	Implement more of the GeoIP library API.
*/


:- dynamic
	geoip_db/2.				% +Name, -Handle

%%	geoip_open(+Name, -Handle) is det.
%
%	True when Handle is a handle  to   the  GeoIP database Name. The
%	database   file   is   searched   for     using    the   pattern
%	`geoip(Name.dat)`. Subsequent calls to this predicate return the
%	handle to the already opened database.

geoip_open(Name, Handle) :-
	geoip_db(Name, Handle), !.
geoip_open(Name, Handle) :-
	with_mutex(geoip,
		   geoip_open_sync(Name, Handle)).

geoip_open_sync(Name, Handle) :-
	geoip_db(Name, Handle), !.
geoip_open_sync(Name, Handle) :-
	file_name_extension(Name, dat, File),
	geoip_open(geoip(File), standard, Handle),
	assertz(geoip_db(Name, Handle)).

%%	geoip_open(+File, +Mode, -Handle) is det.
%
%	Open the GeoIP database file File in   Mode  and return a handle
%	for it. File may use alias   patterns. The default configuration
%	provides the alias  =geoip=,   pointing  to ='/usr/share/GeoIP=.
%	Mode  is  one  of   =standard=,  =memory_cache=,  =check_cache=,
%	=index_cache= or =mmap_cache=. Consult the library documentation
%	for the meaning of these constants.  For example:
%
%	  ==
%	  ?- geoip_open(geoip('GeoIPCity.dat'), standard, H).
%	  H = <geoip>(0x1f3ae30).
%	  ==
%
%	Database handles are subject to (atom)  garbage collection. If a
%	database handle is garbage collected, the underlying database is
%	closed.

%%	geoip_close is det.
%%	geoip_close(?NameOrHandle) is det.
%
%	Close all databases or those matching NameOrHandle.

geoip_close :-
	geoip_close(_).

geoip_close(Name) :-
	( var(Name) ; atom(Name) ), !,
	forall(retract(geoip_db(Name, Handle)),
	       geoip_close_(Handle)).
geoip_close(Handle) :-
	geoip_close_(Handle).

%%	geoip_lookup(+From, -Result) is semidet.
%%	geoip_lookup(+Handle, +From, -Result) is semidet.
%
%	Find  a  record  from   the    GeoIP   database.  The  predicate
%	geoip_lookup/2 uses `geoip_open('GeoIPCity', Handle)`   to get a
%	handle to the GeoIP city database.  Result is a dict, containing
%	a subset of the following keys: =country_code=, =country_code3=,
%	=country_name=,      =region=,      =city=,       =postal_code=,
%	=continent_code=,   =latitude=,   =longitude=,    =netmask=   or
%	=area_code=. This predicate uses   GeoIP_record_by_ipnum()  from
%	the GeoIP library.
%
%	@arg From is either a term `ip(A,B,C,D)`   or a dotted string or
%	atom containing an  ip4  address  or   hostname.  If  From  is a
%	hostname,  tcp_host_to_address/2  is  used  to   lookup  the  IP
%	address.

geoip_lookup(From, Result) :-
	geoip_open('GeoIPCity', Handle),
	geoip_lookup(Handle, From, Result).

geoip_lookup(Handle, From, Result) :-
	to_ip(From, IP),
	geoip_lookup_(Handle, IP, Pairs),
	Pairs \== [],
	dict_pairs(Result, geoip, Pairs).

to_ip(Atomic, IP) :-
	atomic(Atomic), !,
	split_string(Atomic, ".", "", Parts),
	(   length(Parts, 4),
	    maplist(byte_string, Bytes, Parts)
	->  IP =.. [ip|Bytes]
	;   tcp_host_to_address(Atomic, IP)
	).
to_ip(IP, IP).

byte_string(Byte, String) :-
	number_string(Byte, String),
	integer(Byte),
	between(0, 255, Byte).
