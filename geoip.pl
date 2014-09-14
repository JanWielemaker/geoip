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
	    geoip_lookup/3			% +Handle, +From, -Result
	  ]).

:- use_foreign_library(geoip4pl).

:- multifile user:file_search_path/2.

user:file_search_path(geoip, '/usr/share/GeoIP').

:- dynamic
	geoip_db/2.				% +Name, -Handle

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

geoip_lookup(Handle, From, Result) :-
	geoip_lookup_(Handle, From, Pairs),
	Pairs \== [],
	dict_pairs(Result, geoip, Pairs).
