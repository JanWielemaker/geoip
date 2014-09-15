SOBJ=   $(PACKSODIR)/geoip4pl.$(SOEXT)

all:    $(SOBJ)

OBJ=	c/geoip4pl.o

$(SOBJ): $(OBJ)
	mkdir -p $(PACKSODIR)
	$(LD) $(LDSOFLAGS) -o $@ $(OBJ) $(SWISOLIB) -lGeoIP

check::
install::
clean:
	rm -f $(OBJ)
distclean: clean
	rm -f $(SOBJ)
