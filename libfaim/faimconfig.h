/*
 *  faimconfig.h
 *
 * Contains various compile-time options that apply _only_ to libfaim.
 *
 */

#ifndef __FAIMCONFIG_H__
#define __FAIMCONFIG_H__

/*
 * USE_SNAC_FOR_IMS is an old feature that allowed better
 * tracking of error messages by caching SNAC IDs of outgoing
 * ICBMs and comparing them to incoming errors.  However,
 * its a helluvalot of overhead for something that should
 * rarely happen.  
 *
 * Default: defined.  This is now defined by default
 * because it should be stable and its not too bad.  
 * And Josh wanted it.
 *
 */
#define USE_SNAC_FOR_IMS

/*
 * Default Authorizer server name and TCP port for the OSCAR farm.  
 *
 * You shouldn't need to change this unless you're writing
 * your own server. 
 *
 * Note that only one server is needed to start the whole
 * AIM process.  The later server addresses come from
 * the authorizer service.
 *
 * This is only here for convenience.  Its still up to
 * the client to connect to it.
 *
 */
#define FAIM_LOGIN_SERVER "login.oscar.aol.com"
#define FAIM_LOGIN_PORT 5190

/*
 * Size of the SNAC caching hash.
 *
 * Default: 16
 *
 */
#define FAIM_SNAC_HASH_SIZE 16

/*
 * If building on Win32, define WIN32_STATIC if you don't want
 * to compile libfaim as a DLL (and instead link it right into
 * your app).
 */
#define WIN32_STATIC

#endif /* __FAIMCONFIG_H__ */


