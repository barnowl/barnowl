CFLAGS=$(shell pkg-config glib-2.0 --cflags)
LDFLAGS=$(shell pkg-config glib-2.0 --libs)

zcrypt: zcrypt.o filterproc.o

