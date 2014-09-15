#include <SWI-Stream.h>
#include <SWI-Prolog.h>
#include <GeoIP.h>
#include <GeoIPCity.h>
#include <assert.h>

static atom_t ATOM_country_code;
static atom_t ATOM_country_code3;
static atom_t ATOM_country_name;
static atom_t ATOM_region;
static atom_t ATOM_city;
static atom_t ATOM_postal_code;
static atom_t ATOM_latitude;
static atom_t ATOM_longitude;
static atom_t ATOM_dma_code;
static atom_t ATOM_area_code;
static atom_t ATOM_continent_code;
static atom_t ATOM_netmask;

static functor_t FUNCTOR_ip4;
static functor_t FUNCTOR_pair2;


		 /*******************************
		 *	  SYMBOL WRAPPER	*
		 *******************************/

#define GEOIP_MAGIC 746228346


typedef struct geoip_wrapper
{ atom_t	symbol;				/* Associated symbol */
  GeoIP	       *gi;
  int		magic;				/* GEOIP_MAGIC */
} geoip_wrapper;


static void
acquire_geoip(atom_t symbol)
{ geoip_wrapper *gw = PL_blob_data(symbol, NULL, NULL);
  gw->symbol = symbol;
}


static int
release_geoip(atom_t symbol)
{ geoip_wrapper *gw = PL_blob_data(symbol, NULL, NULL);

  if ( gw->gi )
  { GeoIP_delete(gw->gi);
  }

  PL_free(gw);

  return TRUE;
}


static int
compare_geoips(atom_t a, atom_t b)
{ geoip_wrapper *ara = PL_blob_data(a, NULL, NULL);
  geoip_wrapper *arb = PL_blob_data(b, NULL, NULL);

  return ( ara > arb ?  1 :
	   ara < arb ? -1 : 0
	 );
}


static int
write_geoip(IOSTREAM *s, atom_t symbol, int flags)
{ geoip_wrapper *gw = PL_blob_data(symbol, NULL, NULL);

  Sfprintf(s, "<geoip>(%p)", gw);

  return TRUE;
}


static PL_blob_t geoip_blob =
{ PL_BLOB_MAGIC,
  PL_BLOB_NOCOPY,
  "geoip",
  release_geoip,
  compare_geoips,
  write_geoip,
  acquire_geoip
};


static int
get_geoip(term_t t, geoip_wrapper **gwp)
{ PL_blob_t *type;
  void *data;

  if ( PL_get_blob(t, &data, NULL, &type) && type == &geoip_blob)
  { geoip_wrapper *gw = data;

    assert(gw->magic == GEOIP_MAGIC);

    if ( gw->gi )
    { *gwp = gw;

      return TRUE;
    }

    PL_permission_error("access", "closed_geoip_database", t);
    return FALSE;
  }

  return PL_type_error("geoip", t);
}


static foreign_t
geoip_open(term_t File, term_t Flags, term_t handle)
{ char *fn, *fchars;
  int flags;
  GeoIP *gi;

  if ( !PL_get_file_name(File, &fn,
			 PL_FILE_OSPATH|PL_FILE_SEARCH|PL_FILE_READ) ||
       !PL_get_chars(Flags, &fchars, CVT_ATOM|CVT_EXCEPTION) )
    return FALSE;

  if ( strcmp(fchars, "standard") )
    flags = GEOIP_STANDARD;
  else if ( strcmp(fchars, "memory_cache") )
    flags = GEOIP_MEMORY_CACHE;
  else if ( strcmp(fchars, "check_cache") )
    flags = GEOIP_CHECK_CACHE;
  else if ( strcmp(fchars, "index_cache") )
    flags = GEOIP_INDEX_CACHE;
  else if ( strcmp(fchars, "mmap_cache") )
    flags = GEOIP_MMAP_CACHE;
  else
    return PL_domain_error("geoip_flags", Flags);

  if ( (gi=GeoIP_open(fn, flags)) == NULL )
  { return PL_permission_error("open", "geoip", File);	/* TBD: better error */
  } else
  { geoip_wrapper *gw;

    if ( !(gw = PL_malloc(sizeof(*gw))) )
      return FALSE;

    memset(gw, 0, sizeof(*gw));
    gw->gi    = gi;
    gw->magic = GEOIP_MAGIC;
    if ( !PL_unify_blob(handle, gw, sizeof(*gw), &geoip_blob) )
    { GeoIP_delete(gi);
      return FALSE;
    }

    return TRUE;
  }
}


static foreign_t
geoip_close(term_t geoip)
{ geoip_wrapper *gw;

  if ( !get_geoip(geoip, &gw) )
    return FALSE;

  GeoIP_delete(gw->gi);
  gw->gi = NULL;

  return TRUE;
}


static int
string_result(term_t list, atom_t key, const char *val)
{ term_t head = PL_new_term_ref();

  return ( PL_unify_list(list, head, list) &&
	   PL_unify_term(head, PL_FUNCTOR, FUNCTOR_pair2,
			         PL_ATOM, key,
			         PL_CHARS, val) );
}


static int
float_result(term_t list, atom_t key, double val)
{ term_t head = PL_new_term_ref();

  return ( PL_unify_list(list, head, list) &&
	   PL_unify_term(head, PL_FUNCTOR, FUNCTOR_pair2,
			         PL_ATOM, key,
			         PL_FLOAT, val) );
}


static int
int_result(term_t list, atom_t key, int val)
{ term_t head = PL_new_term_ref();

  return ( PL_unify_list(list, head, list) &&
	   PL_unify_term(head, PL_FUNCTOR, FUNCTOR_pair2,
			         PL_ATOM, key,
			         PL_INT, val) );
}


static int
get_ip(term_t from, unsigned long *ipnum)
{ if ( PL_is_functor(from, FUNCTOR_ip4) )
  { term_t arg = PL_new_term_ref();
    int i, p[4];

    for(i=0; i<4; i++)
    { _PL_get_arg(i+1, from, arg);
      if ( !PL_get_integer_ex(arg, &p[i]) )
	return FALSE;
      if ( p[i] < 0 || p[i] > 255 )
	return PL_domain_error("ip4_component", arg);
    }

    *ipnum = (p[0]<<24 | p[1]<<16 | p[2] << 8 | p[3]);
    return TRUE;
  }

  return -1;					/* not an IP4 struct */
}


static foreign_t
geoip_lookup(term_t geoip, term_t from, term_t result)
{ geoip_wrapper *gw;
  unsigned long ipnum;
  term_t tail = PL_copy_term_ref(result);

  if ( !get_geoip(geoip, &gw) )
    return FALSE;
  if ( get_ip(from, &ipnum) == TRUE )
  { GeoIPRecord *gir;

    if ( (gir=GeoIP_record_by_ipnum(gw->gi, ipnum)) )
    { if ( gir->country_code &&
	   !string_result(tail, ATOM_country_code, gir->country_code) )
	return FALSE;
      if ( gir->country_code3 &&
	   !string_result(tail, ATOM_country_code3, gir->country_code3) )
	return FALSE;
      if ( gir->country_name &&
	   !string_result(tail, ATOM_country_name, gir->country_name) )
	return FALSE;
      if ( gir->region &&
	   !string_result(tail, ATOM_region, gir->region) )
	return FALSE;
      if ( gir->city &&
	   !string_result(tail, ATOM_city, gir->city) )
	return FALSE;
      if ( gir->postal_code &&
	   !string_result(tail, ATOM_postal_code, gir->postal_code) )
	return FALSE;
      if ( gir->continent_code &&
	   !string_result(tail, ATOM_continent_code, gir->continent_code) )
	return FALSE;
      if ( !float_result(tail, ATOM_latitude, gir->latitude) )
	return FALSE;
      if ( !float_result(tail, ATOM_longitude, gir->longitude) )
	return FALSE;
      if ( gir->netmask &&
	   !int_result(tail, ATOM_netmask, gir->netmask) )
	return FALSE;
      if ( gir->area_code &&
	   !int_result(tail, ATOM_area_code, gir->area_code) )
	return FALSE;
    }

    return PL_unify_nil(tail);
  } else
    return PL_type_error("ipnum", from);
}


install_t
install_geoip4pl(void)
{ ATOM_country_code   =	PL_new_atom("country_code");
  ATOM_country_code3  =	PL_new_atom("country_code3");
  ATOM_country_name   =	PL_new_atom("country_name");
  ATOM_region	      =	PL_new_atom("region");
  ATOM_city	      =	PL_new_atom("city");
  ATOM_postal_code    =	PL_new_atom("postal_code");
  ATOM_latitude	      =	PL_new_atom("latitude");
  ATOM_longitude      =	PL_new_atom("longitude");
  ATOM_dma_code	      =	PL_new_atom("dma_code");
  ATOM_area_code      =	PL_new_atom("area_code");
  ATOM_continent_code =	PL_new_atom("continent_code");
  ATOM_netmask	      =	PL_new_atom("netmask");

  FUNCTOR_ip4   = PL_new_functor(PL_new_atom("ip"), 4);
  FUNCTOR_pair2 = PL_new_functor(PL_new_atom("-"), 2);

  PL_register_foreign("geoip_open",    3, geoip_open,   0);
  PL_register_foreign("geoip_close_",  1, geoip_close,  0);
  PL_register_foreign("geoip_lookup_", 3, geoip_lookup, 0);
}
