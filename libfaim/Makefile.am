noinst_LIBRARIES = libfaim.a

libfaim_a_SOURCES = admin.c \
     adverts.c auth.c bos.c buddylist.c bstream.c \
     chat.c chatnav.c conn.c email.c ft.c icq.c im.c \
     invite.c md5.c misc.c msgcookie.c locate.c \
     popups.c rxhandlers.c rxqueue.c search.c service.c \
     snac.c ssi.c stats.c tlv.c translate.c txqueue.c \
     util.c odir.c bart.c \
     aim_cbtypes.h  aim.h  aim_internal.h  faimconfig.h md5.h

libfaim_a_CFLAGS = -Wno-pointer-sign
libfaim_a_CPPFLAGS = -DAIM_BUILDDATE=\"x\" -DAIM_BUILDTIME=\"x\" \
                     -I${top_srcdir}/libfaim
