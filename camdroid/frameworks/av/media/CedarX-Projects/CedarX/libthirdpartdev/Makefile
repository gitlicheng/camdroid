LIBPATH=${CEDARX_TOP}/output
BUILDPATH=.

include ${CEDARX_TOP}/../Config-Linux.mk

CFLAGS=-Wall -O2 -D__DISABLE_SFT_ENGIN
CFLAGS+=-I./ -I${CEDARX_TOP}/include/include_cedarv -I${CEDARX_TOP}/include/include_base -I${CEDARX_TOP}/include -I${CEDARX_TOP}/libstream -I${CEDARX_TOP}/libutil 
CFLAGS+=-I${CEDARX_TOP}/include/include_stream -I${CEDARX_TOP}/include/include_demux -I${CEDARX_TOP}/include/include_thirdpartdev
CFLAGS+=$(CEDARX_EXT_CFLAGS) -I${CEDARX_TOP}/include/include_omx

LIBSRC = thirdpart_stream.c

ifeq ($(CEDARX_BUILD_STATIC_LIB),1)
LIB=$(LIBPATH)/libthirdpartstream.a
else
LIB=$(LIBPATH)/libthirdpartstream.so
CFLAGS+=-fPIC
endif


LIBOBJ=$(addprefix $(BUILDPATH)/, $(addsuffix .o, $(basename $(LIBSRC))))

all: $(LIB)

lib:$(LIB)

$(BUILDPATH)/%.o:%.c
	$(CC) $(CFLAGS) $(EXT_CFLAGS) -o $@ -c $<
	
ifeq ($(CEDARX_BUILD_STATIC_LIB),1)
$(LIB):$(LIBOBJ)
	$(AR) -rcs $@ $^
else
$(LIB):$(LIBOBJ)
	$(CC) -shared -o $@ $^
endif


clean:
	rm -f $(LIB) $(LIBOBJ)
	