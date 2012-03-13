use strict;
use warnings;

package BarnOwl::Module::Jabber;

=head1 NAME

BarnOwl::Module::Jabber

=head1 DESCRIPTION

This module implements Jabber support for BarnOwl.

=cut

use BarnOwl;
use BarnOwl::Hooks;

use utf8;

our $VERSION = 0.1;

our $impl_loaded;
$impl_loaded = 0 unless defined($impl_loaded);

sub _load_impl {
    unless ($impl_loaded) {
        BarnOwl::debug("_load_impl");
        require BarnOwl::Module::Jabber::Impl;
        $impl_loaded = 1;
        BarnOwl::Module::Jabber::Impl::onStart();
    }
}

sub onStart {
    if ( *BarnOwl::queue_message{CODE} ) {
        register_owl_commands();
        register_keybindings();
        register_filters();
        $BarnOwl::Hooks::getQuickstart->add("BarnOwl::Module::Jabber::onGetQuickstart");

	BarnOwl::new_variable_bool("jabber:show_offline_buddies",
				   { default => 1,
				     summary => 'Show offline or pending buddies.'});
	BarnOwl::new_variable_bool("jabber:show_logins",
				   { default => 0,
				     summary => 'Show login/logout messages.'});
	BarnOwl::new_variable_bool("jabber:spew",
				   { default => 0,
				     summary => 'Display unrecognized Jabber messages.'});
	BarnOwl::new_variable_int("jabber:auto_away_timeout",
				  { default => 5,
				    summary => 'After minutes idle, auto away.',
				  });
	BarnOwl::new_variable_int("jabber:auto_xa_timeout",
				  { default => 15,
				    summary => 'After minutes idle, auto extended away.'
				});
	BarnOwl::new_variable_bool("jabber:reconnect",
				  { default => 1,
				    summary => 'Auto-reconnect when disconnected from servers.'
				});

        # If we're called as part of module reload, let Impl's reload
        # code run too.
        if ($impl_loaded) {
            BarnOwl::Module::Jabber::Impl::onStart();
        }
    } else {
        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support BarnOwl::error. So just
        # give up silently.
    }
}

$BarnOwl::Hooks::startup->add("BarnOwl::Module::Jabber::onStart");

sub _make_stub {
    my $func = shift;
    return sub {
        _load_impl();
        no strict 'refs';
        &{"BarnOwl::Module::Jabber::Impl::$func"};
    }
}

sub register_owl_commands() {
    BarnOwl::new_command(
        jabberlogin => _make_stub("cmd_login"),
        {
            summary => "Log in to Jabber",
            usage   => "jabberlogin <jid> [<password>]"
        }
    );
    BarnOwl::new_command(
        jabberlogout => _make_stub("cmd_logout"),
        {
            summary => "Log out of Jabber",
            usage   => "jabberlogout [-A|<jid>]",
            description => "jabberlogout logs you out of Jabber.\n\n"
              . "If you are connected to one account, no further arguments are necessary.\n\n"
              . "-A            Log out of all accounts.\n"
              . "<jid>         Which account to log out of.\n"
        }
    );
    BarnOwl::new_command(
        jwrite => _make_stub("cmd_jwrite"),
        {
            summary => "Send a Jabber Message",
            usage   => "jwrite <jid> [-t <thread>] [-s <subject>] [-a <account>] [-m <message>]"
        }
    );
    BarnOwl::new_command(
        jaway => _make_stub("cmd_jaway"),
        {
            summary => "Set Jabber away / presence information",
            usage   => "jaway [-s online|dnd|...] [<message>]"
        }
    );
    BarnOwl::new_command(
        jlist => _make_stub("cmd_jlist"),
        {
            summary => "Show your Jabber roster.",
            usage   => "jlist"
        }
    );
    BarnOwl::new_command(
        jmuc => _make_stub("cmd_jmuc"),
        {
            summary     => "Jabber MUC related commands.",
            description => "jmuc sends Jabber commands related to MUC.\n\n"
              . "The following commands are available\n\n"
              . "join <muc>[/<nick>]\n"
              . "            Join a MUC (with a given nickname, or otherwise your JID).\n\n"
              . "part <muc>  Part a MUC.\n"
              . "            The MUC is taken from the current message if not supplied.\n\n"
              . "invite <jid> [<muc>]\n"
              . "            Invite <jid> to <muc>.\n"
              . "            The MUC is taken from the current message if not supplied.\n\n"
              . "configure [<muc>]\n"
              . "            Configures a MUC.\n"
              . "            Necessary to initalize a new MUC.\n"
              . "            At present, only the default configuration is supported.\n"
              . "            The MUC is taken from the current message if not supplied.\n\n"
              . "presence [<muc>]\n"
              . "            Shows the roster for <muc>.\n"
              . "            The MUC is taken from the current message if not supplied.\n\n"
              . "presence -a\n"
              . "            Shows rosters for all MUCs you're participating in.\n\n",
            usage => "jmuc <command> [<args>]"
        }
    );
    BarnOwl::new_command(
        jroster => _make_stub("cmd_jroster"),
        {
            summary     => "Jabber roster related commands.",
            description => "jroster sends Jabber commands related to rosters.\n\n"
              . "The following commands are available\n\n"
              . "sub <jid>     Subscribe to <jid>'s presence. (implicit add)\n\n"
              . "add <jid>     Adds <jid> to your roster.\n\n"
              . "unsub <jid>   Unsubscribe from <jid>'s presence.\n\n"
              . "remove <jid>  Removes <jid> from your roster. (implicit unsub)\n\n"
              . "auth <jid>    Authorizes <jid> to subscribe to your presence.\n\n"
              . "deauth <jid>  De-authorizes <jid>'s subscription to your presence.\n\n"
              . "The following arguments are supported for all commands\n\n"
              . "-a <jid>      Specify which account to make the roster changes on.\n"
              . "              Required if you're signed into more than one account.\n\n"
              . "The following arguments only work with the add and sub commands.\n\n"
              . "-g <group>    Add <jid> to group <group>.\n"
              . "              May be specified more than once, will not remove <jid> from any groups.\n\n"
              . "-p            Purge. Removes <jid> from all groups.\n"
              . "              May be combined with -g.\n\n"
              . "-n <name>     Sets <name> as <jid>'s short name.\n\n"
              . "Note: Unless -n is used, you can specify multiple <jid> arguments.\n",
            usage       => "jroster <command> <args>"
        }
    );
}

sub register_keybindings {
    BarnOwl::bindkey(qw(recv j command start-command), 'jwrite ');
}

sub register_filters {
    BarnOwl::filter(qw(jabber type ^jabber$));
}

sub onGetQuickstart {
    return <<'EOF'
@b(Jabber:)
Type ':jabberlogin @b(username@mit.edu)' to log in to Jabber. The command
':jroster sub @b(somebody@gmail.com)' will request that they let you message
them. Once you get a message saying you are subscribed, you can message
them by typing ':jwrite @b(somebody@gmail.com)' or just 'j @b(somebody)'.
EOF
}

1;
