include Makefile.local

.PHONY: libhashutil5 birthdaysearch birthdaysearch_cuda diffpathforward diffpathbackward diffpathconnect diffpathhelper 
.PHONY: all test proper clean boost boost-wget-untar boost-build boost-strip

all: libhashutil5 birthdaysearch diffpathforward diffpathbackward diffpathconnect diffpathhelper birthdaysearch_cuda 

libhashutil5: boost-once test
	@cd libhashutil5 && $(MAKE)

birthdaysearch: libhashutil5
	@cd birthdaysearch && $(MAKE)

birthdaysearch_cuda: libhashutil5
	@(cd birthdaysearch && test -n "$(CUDA_TK)" && $(MAKE) -f Makefile.cuda || echo) || echo "Not building birthdaysearch_cuda: CUDA Disabled."
	
diffpathforward: libhashutil5
	@cd diffpathforward && $(MAKE)

diffpathbackward: libhashutil5
	@cd diffpathbackward && $(MAKE)

diffpathconnect: libhashutil5
	@cd diffpathconnect && $(MAKE)

diffpathhelper: libhashutil5
	@cd diffpathhelper && $(MAKE)

proper clean:
	cd libhashutil5 && $(MAKE) $@
	cd birthdaysearch && $(MAKE) $@
	cd birthdaysearch && $(MAKE) -f Makefile.cuda $@
	cd diffpathforward && $(MAKE) $@
	cd diffpathbackward && $(MAKE) $@
	cd diffpathconnect && $(MAKE) $@
	cd diffpathhelper && $(MAKE) $@

boost-once:
	[ -d boost_$(BOOST_LIBVER)/stage/lib ] || $(MAKE) -j boost
	
boost: boost-strip
boost-strip: boost-build
boost-build: boost-wget-untar

boost-wget-untar:
	[ -f boost_$(BOOST_LIBVER).tar.bz2 ] || wget -c http://ftp.osuosl.org/pub/blfs/conglomeration/boost/boost_$(BOOST_LIBVER).tar.bz2 || (echo "ERROR: manually download boost_$(BOOST_LIBVER).tar.bz2 !!"; false)
	tar -xjvf boost_$(BOOST_LIBVER).tar.bz2

boost-build:
	cd boost_$(BOOST_LIBVER) && \
	./bootstrap.sh --with-libraries=date_time,filesystem,iostreams,program_options,serialization,system,thread && \
	./bjam -j $(CPUS) cxxflags="$(CPPFLAGS)" release threading=multi link=static

boost-strip:
	cd boost_$(BOOST_LIBVER) && \
	rm -rf `ls | grep -v boost | grep -v stage` && \
	rm boost* 2>/dev/null || true

test:
	@cd libhashutil5 && \
	(echo "#include <boost/cstdint.hpp>" | $(CPP) $(CPPFLAGS) -x c++ -c - -o /dev/null 2>/dev/null >/dev/null) || \
	(echo "ERROR: Cannot compile with your current configuration in Makefile.local:" && (echo "#include <boost/cstdint.hpp>" | $(CPP) $(CPPFLAGS) -x c++ -c - -o /dev/null ))
