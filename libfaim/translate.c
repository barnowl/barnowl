/*
 * Family 0x000c - Translation.
 *
 * I have no idea why this group was issued.  I have never seen anything
 * that uses it.  From what I remember, the last time I tried to poke at
 * the server with this group, it whined about not supporting it.
 *
 * But we advertise it anyway, because its fun.
 * 
 */

#define FAIM_INTERNAL
#include <aim.h>

faim_internal int translate_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x000c;
	mod->version = 0x0001;
	mod->toolid = 0x0104;
	mod->toolversion = 0x0001;
	mod->flags = 0;
	strncpy(mod->name, "translate", sizeof(mod->name));
	mod->snachandler = NULL;

	return 0;
}
