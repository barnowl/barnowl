/*
 * Family 0x0006 - This isn't really ever used by anyone anymore.
 *
 * Once upon a time, there used to be a menu item in AIM clients that
 * said something like "Invite a friend to use AIM..." and then it would
 * ask for an email address and it would sent a mail to them saying
 * how perfectly wonderful the AIM service is and why you should use it
 * and click here if you hate the person who sent this to you and want to
 * complain and yell at them in a small box with pretty fonts.
 *
 * I could've sworn libfaim had this implemented once, a long long time ago,
 * but I can't find it.
 *
 * I'm mainly adding this so that I can keep advertising that we support
 * group 6, even though we don't.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

faim_internal int invite_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0006;
	mod->version = 0x0001;
	mod->toolid = 0x0110;
	mod->toolversion = 0x0629;
	mod->flags = 0;
	strncpy(mod->name, "invite", sizeof(mod->name));
	mod->snachandler = NULL;

	return 0;
}
