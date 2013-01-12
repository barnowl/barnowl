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
use BarnOwl::Message::Jabber;
use BarnOwl::Module::Jabber::Connection;
use BarnOwl::Module::Jabber::ConnectionManager;
use BarnOwl::Completion::Util qw(complete_flags);

use Authen::SASL qw(Perl);
use Net::Jabber;
use Net::Jabber::MUC;
use Net::DNS;
use Getopt::Long;
Getopt::Long::Configure(qw(no_getopt_compat prefix_pattern=-|--));

use utf8;

our $VERSION = 0.1;

BEGIN {
    if(eval {require IO::Socket::SSL;}) {
        if($IO::Socket::SSL::VERSION eq "0.97") {
            BarnOwl::error("You are using IO::Socket:SSL 0.97, which \n" .
                           "contains bugs causing it not to work with BarnOwl's\n" .
                           "Jabber support. We recommend updating to the latest\n" .
                           "IO::Socket::SSL from CPAN. \n");
            die("Not loading Jabber.par\n");
        }
    }
}

no warnings 'redefine';

################################################################################
# owl perl jabber support
#
# XXX Todo:
# Rosters for MUCs
# More user feedback
#  * joining MUC
#  * parting MUC
#  * presence (Roster and MUC)
# Implementing formatting and logging callbacks for C
# Appropriate callbacks for presence subscription messages.
#
################################################################################

our $conn;
$conn ||= BarnOwl::Module::Jabber::ConnectionManager->new;
our %vars;
our %completion_jids;

sub onStart {
    if ( *BarnOwl::queue_message{CODE} ) {
        register_owl_commands();
        register_keybindings();
        register_filters();
        $BarnOwl::Hooks::getBuddyList->add("BarnOwl::Module::Jabber::onGetBuddyList");
        $BarnOwl::Hooks::getQuickstart->add("BarnOwl::Module::Jabber::onGetQuickstart");
        $vars{show} = '';
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
        # Force these. Reload can screw them up.
        # Taken from Net::Jabber::Protocol.
        $Net::XMPP::Protocol::NEWOBJECT{'iq'}       = "Net::Jabber::IQ";
        $Net::XMPP::Protocol::NEWOBJECT{'message'}  = "Net::Jabber::Message";
        $Net::XMPP::Protocol::NEWOBJECT{'presence'} = "Net::Jabber::Presence";
        $Net::XMPP::Protocol::NEWOBJECT{'jid'}      = "Net::Jabber::JID";
    } else {
        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support BarnOwl::error. So just
        # give up silently.
    }
}

$BarnOwl::Hooks::startup->add("BarnOwl::Module::Jabber::onStart");

sub do_keep_alive_and_auto_away {
    if ( !$conn->connected() ) {
        # We don't need this timer any more.
        if (defined $vars{keepAliveTimer}) {
            $vars{keepAliveTimer}->stop;
        }
        delete $vars{keepAliveTimer};
        return;
    }

    $vars{status_changed} = 0;
    my $auto_away = BarnOwl::getvar('jabber:auto_away_timeout');
    my $auto_xa = BarnOwl::getvar('jabber:auto_xa_timeout');
    my $idletime = BarnOwl::getidletime();
    if ($auto_xa != 0 && $idletime >= (60 * $auto_xa) && ($vars{show} eq 'away' || $vars{show} eq '' )) {
        $vars{show} = 'xa';
        $vars{status} = 'Auto extended-away after '.$auto_xa.' minute'.($auto_xa == 1 ? '' : 's').' idle.';
        $vars{status_changed} = 1;
    } elsif ($auto_away != 0 && $idletime >= (60 * $auto_away) && $vars{show} eq '') {
        $vars{show} = 'away';
        $vars{status} = 'Auto away after '.$auto_away.' minute'.($auto_away == 1 ? '' : 's').' idle.';
        $vars{status_changed} = 1;
    } elsif ($idletime <= $vars{idletime} && $vars{show} ne '') {
        $vars{show} = '';
        $vars{status} = '';
        $vars{status_changed} = 1;
    }
    $vars{idletime} = $idletime;

    foreach my $jid ( $conn->getJIDs() ) {
        next if $conn->jidActive($jid);
        $conn->tryReconnect($jid);
    }

    foreach my $jid ( $conn->getJIDs() ) {
        next unless $conn->jidActive($jid);

        my $client = $conn->getConnectionFromJID($jid);
        unless($client) {
            $conn->removeConnection($jid);
            BarnOwl::error("Connection for $jid undefined -- error in reload?");
        }
        my $status = $client->Process(0); # keep-alive
        if ( !defined($status) ) {
            $conn->scheduleReconnect($jid);
        }
        if ($::shutdown) {
            do_logout($jid);
            next;
        }

        if ($vars{status_changed}) {
            my $p = new Net::Jabber::Presence;
            $p->SetShow($vars{show}) if $vars{show};
            $p->SetStatus($vars{status}) if $vars{status};
            $client->Send($p);
        }
    }
}

our $showOffline = 0;

sub blist_listBuddy {
    my $roster = shift;
    my $buddy  = shift;
    my $blistStr .= "    ";
    my %jq  = $roster->query($buddy);
    my $res = $roster->resource($buddy);

    my $name = $jq{name} || $buddy->GetUserID();

    $blistStr .= sprintf '%-15s %s', $name, $buddy->GetJID();
    $completion_jids{$name} = 1;
    $completion_jids{$buddy->GetJID()} = 1;

    if ($res) {
        my %rq = $roster->resourceQuery( $buddy, $res );
        $blistStr .= " [" . ( $rq{show} ? $rq{show} : 'online' ) . "]";
        $blistStr .= " " . $rq{status} if $rq{status};
        $blistStr = BarnOwl::Style::boldify($blistStr) if $showOffline;
    }
    else {
        return '' unless $showOffline;
	if ($jq{ask}) {
            $blistStr .= " [pending]";
	}
	elsif ($jq{subscription} eq 'none' || $jq{subscription} eq 'from') {
	    $blistStr .= " [not subscribed]";
	}
	else {
	    $blistStr .= " [offline]";
	}
    }
    return $blistStr . "\n";
}

# Sort, ignoring markup.
sub blistSort {
    return uc(BarnOwl::ztext_stylestrip($a)) cmp uc(BarnOwl::ztext_stylestrip($b));
}

sub getSingleBuddyList {
    my $jid = shift;
    $jid = resolveConnectedJID($jid);
    return "" unless $jid;
    my $blist = "";
    my $roster = $conn->getRosterFromJID($jid);
    if ($roster) {
        $blist .= BarnOwl::Style::boldify("Jabber roster for $jid\n");

        my @gTexts = ();
        foreach my $group ( $roster->groups() ) {
            my @buddies = $roster->jids( 'group', $group );
            my @bTexts = ();
            foreach my $buddy ( @buddies ) {
                push(@bTexts, blist_listBuddy( $roster, $buddy ));
            }
            push(@gTexts, "  Group: $group\n".join('',sort blistSort @bTexts));
        }
        # Sort groups before adding ungrouped entries.
        @gTexts = sort blistSort @gTexts;

        my @unsorted = $roster->jids('nogroup');
        if (@unsorted) {
            my @bTexts = ();
            foreach my $buddy (@unsorted) {
                push(@bTexts, blist_listBuddy( $roster, $buddy ));
            }
            push(@gTexts, "  [unsorted]\n".join('',sort blistSort @bTexts));
        }
        $blist .= join('', @gTexts);
    }
    return $blist;
}

sub onGetBuddyList {
    $showOffline = BarnOwl::getvar('jabber:show_offline_buddies') eq 'on';
    my $blist = "";
    foreach my $jid ($conn->getJIDs()) {
        $blist .= getSingleBuddyList($jid);
    }
    return $blist;
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

################################################################################
### Owl Commands
sub register_owl_commands() {
    BarnOwl::new_command(
        jabberlogin => \&cmd_login,
        {
            summary => "Log in to Jabber",
            usage   => "jabberlogin <jid> [<password>]"
        }
    );
    BarnOwl::new_command(
        jabberlogout => \&cmd_logout,
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
        jwrite => \&cmd_jwrite,
        {
            summary => "Send a Jabber Message",
            usage   => "jwrite <jid> [-t <thread>] [-s <subject>] [-a <account>] [-m <message>]"
        }
    );
    BarnOwl::new_command(
        jaway => \&cmd_jaway,
        {
            summary => "Set Jabber away / presence information",
            usage   => "jaway [-s online|dnd|...] [<message>]"
        }
    );
    BarnOwl::new_command(
        jlist => \&cmd_jlist,
        {
            summary => "Show your Jabber roster.",
            usage   => "jlist"
        }
    );
    BarnOwl::new_command(
        jmuc => \&cmd_jmuc,
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
              . "            Necessary to initialize a new MUC.\n"
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
        jroster => \&cmd_jroster,
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

sub cmd_login {
    my $cmd = shift;
    my $jidStr = shift;
    my $jid = new Net::Jabber::JID;
    $jid->SetJID($jidStr);
    my $password = '';
    $password = shift if @_;

    my $uid           = $jid->GetUserID();
    my $componentname = $jid->GetServer();
    my $resource      = $jid->GetResource();

    if ($resource eq '') {
        my $cjidStr = $conn->baseJIDExists($jidStr);
        if ($cjidStr) {
            die("Already logged in as $cjidStr.\n");
        }
    }

    $resource ||= 'barnowl';
    $jid->SetResource($resource);
    $jidStr = $jid->GetJID('full');

    if ( !$uid || !$componentname ) {
        die("usage: $cmd JID\n");
    }

    if ( $conn->jidActive($jidStr) ) {
        die("Already logged in as $jidStr.\n");
    } elsif ($conn->jidExists($jidStr)) {
        return $conn->tryReconnect($jidStr, 1);
    }

    my ( $server, $port ) = getServerFromJID($jid);

    $vars{jlogin_jid} = $jidStr;
    $vars{jlogin_connhash} = {
        hostname      => $server,
        tls           => 1,
        port          => $port,
        componentname => $componentname
    };
    $vars{jlogin_authhash} =
      { username => $uid,
        resource => $resource,
    };

    return do_login($password);
}

sub do_login {
    $vars{jlogin_password} = shift;
    $vars{jlogin_authhash}->{password} = sub { return $vars{jlogin_password} || '' };
    my $jidStr = $vars{jlogin_jid};
    if ( !$jidStr && $vars{jlogin_havepass}) {
        BarnOwl::error("Got password but have no JID!");
    }
    else
    {
        my $client = $conn->addConnection($jidStr);

        #XXX Todo: Add more callbacks.
        # * MUC presence handlers
        # We use the anonymous subrefs in order to have the correct behavior
        # when we reload
        $client->SetMessageCallBacks(
            chat      => sub { BarnOwl::Module::Jabber::process_incoming_chat_message(@_) },
            error     => sub { BarnOwl::Module::Jabber::process_incoming_error_message(@_) },
            groupchat => sub { BarnOwl::Module::Jabber::process_incoming_groupchat_message(@_) },
            headline  => sub { BarnOwl::Module::Jabber::process_incoming_headline_message(@_) },
            normal    => sub { BarnOwl::Module::Jabber::process_incoming_normal_message(@_) }
        );
        $client->SetPresenceCallBacks(
            available    => sub { BarnOwl::Module::Jabber::process_presence_available(@_) },
            unavailable  => sub { BarnOwl::Module::Jabber::process_presence_available(@_) },
            subscribe    => sub { BarnOwl::Module::Jabber::process_presence_subscribe(@_) },
            subscribed   => sub { BarnOwl::Module::Jabber::process_presence_subscribed(@_) },
            unsubscribe  => sub { BarnOwl::Module::Jabber::process_presence_unsubscribe(@_) },
            unsubscribed => sub { BarnOwl::Module::Jabber::process_presence_unsubscribed(@_) },
            error        => sub { BarnOwl::Module::Jabber::process_presence_error(@_) });

        my $status = $client->Connect( %{ $vars{jlogin_connhash} } );
        if ( !$status ) {
            $conn->removeConnection($jidStr);
            BarnOwl::error("We failed to connect.");
        } else {
            my @result = $client->AuthSend( %{ $vars{jlogin_authhash} } );

            if ( !@result || $result[0] ne 'ok' ) {
                if ( !$vars{jlogin_havepass} && ( !@result || $result[0] eq '401' || $result[0] eq 'error') ) {
                    $vars{jlogin_havepass} = 1;
                    $conn->removeConnection($jidStr);
                    BarnOwl::start_password("Password for $jidStr: ", \&do_login );
                    return "";
                }
                $conn->removeConnection($jidStr);
                BarnOwl::error( "Error in connect: " . join( " ", @result ) );
            } else {
                $conn->setAuth(
                    $jidStr,
                    {   %{ $vars{jlogin_authhash} },
                        password => $vars{jlogin_password}
                    }
                );
                $client->onConnect($conn, $jidStr);
            }
        }
    }
    delete $vars{jlogin_jid};
    $vars{jlogin_password} =~ tr/\0-\377/x/ if $vars{jlogin_password};
    delete $vars{jlogin_password};
    delete $vars{jlogin_havepass};
    delete $vars{jlogin_connhash};
    delete $vars{jlogin_authhash};

    return "";
}

sub do_logout {
    my $jid = shift;
    my $disconnected = $conn->removeConnection($jid);
    queue_admin_msg("Jabber disconnected ($jid).") if $disconnected;
}

sub cmd_logout {
    return "You are not logged into Jabber." unless ($conn->connected() > 0);
    # Logged into multiple accounts
    if ( $conn->connected() > 1 ) {
        # Logged into multiple accounts, no accout specified.
        if ( !$_[1] ) {
            my $errStr =
              "You are logged into multiple accounts. Please specify an account to log out of.\n";
            foreach my $jid ( $conn->getJIDs() ) {
                $errStr .= "\t$jid\n";
            }
            queue_admin_msg($errStr);
        }
        # Logged into multiple accounts, account specified.
        else {
            if ( $_[1] eq '-A' )    #All accounts.
            {
                foreach my $jid ( $conn->getJIDs() ) {
                    do_logout($jid);
                }
            }
            else                    #One account.
            {
                my $jid = resolveConnectedJID( $_[1] );
                do_logout($jid) if ( $jid ne '' );
            }
        }
    }
    else                            # Only one account logged in.
    {
        do_logout( ( $conn->getJIDs() )[0] );
    }
    return "";
}

sub cmd_jlist {
    if ( !( scalar $conn->getJIDs() ) ) {
        die("You are not logged in to Jabber.\n");
    }
    BarnOwl::popless_ztext( onGetBuddyList() );
}

sub cmd_jwrite {
    if ( !$conn->connected() ) {
        die("You are not logged in to Jabber.\n");
    }

    my $jwrite_to      = "";
    my $jwrite_from    = "";
    my $jwrite_sid     = "";
    my $jwrite_thread  = "";
    my $jwrite_subject = "";
    my $jwrite_body;
    my ($to, $from);
    my $jwrite_type    = "chat";

    my @args = @_;
    shift;
    local @ARGV = @_;
    my $gc;
    GetOptions(
        'thread=s'  => \$jwrite_thread,
        'subject=s' => \$jwrite_subject,
        'account=s' => \$from,
        'id=s'      => \$jwrite_sid,
        'message=s' => \$jwrite_body,
    ) or die("Usage: jwrite <jid> [-t <thread>] [-s <subject>] [-a <account>]\n");
    $jwrite_type = 'groupchat' if $gc;

    if ( scalar @ARGV != 1 ) {
        die("Usage: jwrite <jid> [-t <thread>] [-s <subject>] [-a <account>]\n");
    }
    else {
      $to = shift @ARGV;
    }

    my @candidates = guess_jwrite($from, $to);

    unless(scalar @candidates) {
        die("Unable to resolve JID $to\n");
    }

    @candidates = grep {defined $_->[0]} @candidates;

    unless(scalar @candidates) {
        if(!$from) {
            die("You must specify an account with -a\n");
        } else {
            die("Unable to resolve account $from\n");
        }
    }


    ($jwrite_from, $jwrite_to, $jwrite_type) = @{$candidates[0]};

    $vars{jwrite} = {
        to      => $jwrite_to,
        from    => $jwrite_from,
        sid     => $jwrite_sid,
        subject => $jwrite_subject,
        thread  => $jwrite_thread,
        type    => $jwrite_type
    };

    if (defined($jwrite_body)) {
        process_owl_jwrite($jwrite_body);
        return;
    }

    if(scalar @candidates > 1) {
        BarnOwl::message(
            "Warning: Guessing account and/or destination JID"
           );
    } else  {
        BarnOwl::message(
            "Type your message below.  End with a dot on a line by itself.  ^C will quit."
           );
    }

    my @cmd = ('jwrite', $jwrite_to, '-a', $jwrite_from);
    push @cmd, '-t', $jwrite_thread if $jwrite_thread;
    push @cmd, '-s', $jwrite_subject if $jwrite_subject;

    BarnOwl::start_edit_win(BarnOwl::quote(@cmd), \&process_owl_jwrite);
}

sub cmd_jmuc {
    die "You are not logged in to Jabber" unless $conn->connected();
    my $ocmd = shift;
    my $cmd  = shift;
    if ( !$cmd ) {

        #XXX TODO: Write general usage for jmuc command.
        return;
    }

    my %jmuc_commands = (
        join      => \&jmuc_join,
        part      => \&jmuc_part,
        invite    => \&jmuc_invite,
        configure => \&jmuc_configure,
        presence  => \&jmuc_presence
    );
    my $func = $jmuc_commands{$cmd};
    if ( !$func ) {
        die("jmuc: Unknown command: $cmd\n");
    }

    {
        local @ARGV = @_;
        my $jid;
        my $muc;
        my $m = BarnOwl::getcurmsg();
        if ( $m && $m->is_jabber && $m->{jtype} eq 'groupchat' ) {
            $muc = $m->{room};
            $jid = $m->{to};
        }

        my $getopt = Getopt::Long::Parser->new;
        $getopt->configure('pass_through', 'no_getopt_compat');
        $getopt->getoptions( 'account=s' => \$jid );
        $jid ||= defaultJID();
        if ($jid) {
            $jid = resolveConnectedJID($jid);
            return unless $jid;
        }
        else {
            die("You must specify an account with -a <jid>\n");
        }
        return $func->( $jid, $muc, @ARGV );
    }
}

sub jmuc_join {
    my ( $jid, $muc, @args ) = @_;
    local @ARGV = @args;
    my $password;
    GetOptions( 'password=s' => \$password );

    $muc = shift @ARGV
      or die("Usage: jmuc join <muc> [-p <password>] [-a <account>]\n");

    die("Error: Must specify a fully-qualified MUC name (e.g. barnowl\@conference.mit.edu)\n")
        unless $muc =~ /@/;
    $muc = Net::Jabber::JID->new($muc);
    $jid = Net::Jabber::JID->new($jid);
    $muc->SetResource($jid->GetJID('full')) unless length $muc->GetResource();

    $conn->getConnectionFromJID($jid)->MUCJoin(JID      => $muc,
                                               Password => $password,
                                               History  => {
                                                   MaxChars => 0
                                                  });
    $completion_jids{$muc->GetJID('base')} = 1;
    return;
}

sub jmuc_part {
    my ( $jid, $muc, @args ) = @_;

    $muc = shift @args if scalar @args;
    die("Usage: jmuc part [<muc>] [-a <account>]\n") unless $muc;

    if($conn->getConnectionFromJID($jid)->MUCLeave(JID => $muc)) {
        queue_admin_msg("$jid has left $muc.");
    } else {
        die("Error: Not joined to $muc\n");
    }
}

sub jmuc_invite {
    my ( $jid, $muc, @args ) = @_;

    my $invite_jid = shift @args;
    $muc = shift @args if scalar @args;

    die("Usage: jmuc invite <jid> [<muc>] [-a <account>]\n")
      unless $muc && $invite_jid;

    my $message = Net::Jabber::Message->new();
    $message->SetTo($muc);
    my $x = $message->NewChild('http://jabber.org/protocol/muc#user');
    $x->AddInvite();
    $x->GetInvite()->SetTo($invite_jid);
    $conn->getConnectionFromJID($jid)->Send($message);
    queue_admin_msg("$jid has invited $invite_jid to $muc.");
}

sub jmuc_configure {
    my ( $jid, $muc, @args ) = @_;
    $muc = shift @args if scalar @args;
    die("Usage: jmuc configure [<muc>]\n") unless $muc;
    my $iq = Net::Jabber::IQ->new();
    $iq->SetTo($muc);
    $iq->SetType('set');
    my $query = $iq->NewQuery("http://jabber.org/protocol/muc#owner");
    my $x     = $query->NewChild("jabber:x:data");
    $x->SetType('submit');

    $conn->getConnectionFromJID($jid)->Send($iq);
    queue_admin_msg("Accepted default instant configuration for $muc");
}

sub jmuc_presence_single {
    my $m = shift;
    my @jids = $m->Presence();

    my $presence = "JIDs present in " . $m->BaseJID;
    $completion_jids{$m->BaseJID} = 1;
    if($m->Anonymous) {
        $presence .= " [anonymous MUC]";
    }
    $presence .= "\n\t";
    $presence .= join("\n\t", map {pp_jid($m, $_);} @jids) . "\n";
    return $presence;
}

sub pp_jid {
    my ($m, $jid) = @_;
    my $nick = $jid->GetResource;
    my $full = $m->GetFullJID($jid);
    if($full && $full ne $nick) {
        return "$nick ($full)";
    } else {
        return "$nick";
    }
}

sub jmuc_presence {
    my ( $jid, $muc, @args ) = @_;

    $muc = shift @args if scalar @args;
    die("Usage: jmuc presence [<muc>]\n") unless $muc;

    if ($muc eq '-a') {
        my $str = "";
        foreach my $jid ($conn->getJIDs()) {
            $str .= BarnOwl::Style::boldify("Conferences for $jid:\n");
            my $connection = $conn->getConnectionFromJID($jid);
            foreach my $muc ($connection->MUCs) {
                $str .= jmuc_presence_single($muc)."\n";
            }
        }
        BarnOwl::popless_ztext($str);
    }
    else {
        my $m = $conn->getConnectionFromJID($jid)->FindMUC(jid => $muc);
        die("No such muc: $muc\n") unless $m;
        BarnOwl::popless_ztext(jmuc_presence_single($m));
    }
}


#XXX TODO: Consider merging this with jmuc and selecting off the first two args.
sub cmd_jroster {
    die "You are not logged in to Jabber" unless $conn->connected();
    my $ocmd = shift;
    my $cmd  = shift;
    if ( !$cmd ) {

        #XXX TODO: Write general usage for jroster command.
        return;
    }

    my %jroster_commands = (
        sub      => \&jroster_sub,
        unsub    => \&jroster_unsub,
        add      => \&jroster_add,
        remove   => \&jroster_remove,
        auth     => \&jroster_auth,
        deauth   => \&jroster_deauth
    );
    my $func = $jroster_commands{$cmd};
    if ( !$func ) {
        die("jroster: Unknown command: $cmd\n");
    }

    {
        local @ARGV = @_;
        my $jid;
        my $name;
        my @groups;
        my $purgeGroups;
        my $getopt = Getopt::Long::Parser->new;
        $getopt->configure('pass_through', 'no_getopt_compat');
        $getopt->getoptions(
            'account=s' => \$jid,
            'group=s' => \@groups,
            'purgegroups' => \$purgeGroups,
            'name=s' => \$name
        );
        $jid ||= defaultJID();
        if ($jid) {
            $jid = resolveConnectedJID($jid);
            return unless $jid;
        }
        else {
            die("You must specify an account with -a <jid>\n");
        }
        return $func->( $jid, $name, \@groups, $purgeGroups,  @ARGV );
    }
}

sub cmd_jaway {
    my $cmd = shift;
    local @ARGV = @_;
    my $getopt = Getopt::Long::Parser->new;
    my ($jid, $show);
    my $p = new Net::Jabber::Presence;

    $getopt->configure('pass_through', 'no_getopt_compat');
    $getopt->getoptions(
        'account=s' => \$jid,
        'show=s'    => \$show
    );
    $jid ||= defaultJID();
    if ($jid) {
        $jid = resolveConnectedJID($jid);
        return unless $jid;
    }
    else {
        die("You must specify an account with -a <jid>\n");
    }

    $p->SetShow($show eq "online" ? "" : $show) if $show;
    $p->SetStatus(join(' ', @ARGV)) if @ARGV;
    $conn->getConnectionFromJID($jid)->Send($p);
}


sub jroster_sub {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJID = baseJID($jid);

    my $roster = $conn->getRosterFromJID($jid);

    # Adding lots of users with the same name is a bad idea.
    $name = "" unless (1 == scalar(@ARGV));

    my $p = new Net::Jabber::Presence;
    $p->SetType('subscribe');

    foreach my $to (@ARGV) {
        jroster_add($jid, $name, \@groups, $purgeGroups, ($to)) unless ($roster->exists($to));

        $p->SetTo($to);
        $conn->getConnectionFromJID($jid)->Send($p);
        queue_admin_msg("You ($baseJID) have requested a subscription to ($to)'s presence.");
    }
}

sub jroster_unsub {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJID = baseJID($jid);

    my $p = new Net::Jabber::Presence;
    $p->SetType('unsubscribe');
    foreach my $to (@ARGV) {
        $p->SetTo($to);
        $conn->getConnectionFromJID($jid)->Send($p);
        queue_admin_msg("You ($baseJID) have unsubscribed from ($to)'s presence.");
    }
}

sub jroster_add {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJID = baseJID($jid);

    my $roster = $conn->getRosterFromJID($jid);

    # Adding lots of users with the same name is a bad idea.
    $name = "" unless (1 == scalar(@ARGV));

    $completion_jids{$baseJID} = 1;
    $completion_jids{$name} = 1 if $name;

    foreach my $to (@ARGV) {
        my %jq  = $roster->query($to);
        my $iq = new Net::Jabber::IQ;
        $iq->SetType('set');
        my $item = new XML::Stream::Node('item');
        $iq->NewChild('jabber:iq:roster')->AddChild($item);

        my %allGroups = ();

        foreach my $g (@groups) {
            $allGroups{$g} = $g;
        }

        unless ($purgeGroups) {
            foreach my $g (@{$jq{groups}}) {
                $allGroups{$g} = $g;
            }
        }

        foreach my $g (keys %allGroups) {
            $item->add_child('group')->add_cdata($g);
        }

        $item->put_attrib(jid => $to);
        $item->put_attrib(name => $name) if $name;
        $conn->getConnectionFromJID($jid)->Send($iq);
        my $msg = "$baseJID: "
          . ($name ? "$name ($to)" : "($to)")
          . " is on your roster in the following groups: { "
          . join(" , ", keys %allGroups)
          . " }";
        queue_admin_msg($msg);
    }
}

sub jroster_remove {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJID = baseJID($jid);

    my $iq = new Net::Jabber::IQ;
    $iq->SetType('set');
    my $item = new XML::Stream::Node('item');
    $iq->NewChild('jabber:iq:roster')->AddChild($item);
    $item->put_attrib(subscription=> 'remove');
    foreach my $to (@ARGV) {
        $item->put_attrib(jid => $to);
        $conn->getConnectionFromJID($jid)->Send($iq);
        queue_admin_msg("You ($baseJID) have removed ($to) from your roster.");
    }
}

sub jroster_auth {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJID = baseJID($jid);

    my $p = new Net::Jabber::Presence;
    $p->SetType('subscribed');
    foreach my $to (@ARGV) {
        $p->SetTo($to);
        $conn->getConnectionFromJID($jid)->Send($p);
        queue_admin_msg("($to) has been subscribed to your ($baseJID) presence.");
    }
}

sub jroster_deauth {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJID = baseJID($jid);

    my $p = new Net::Jabber::Presence;
    $p->SetType('unsubscribed');
    foreach my $to (@ARGV) {
        $p->SetTo($to);
        $conn->getConnectionFromJID($jid)->Send($p);
        queue_admin_msg("($to) has been unsubscribed from your ($baseJID) presence.");
    }
}

################################################################################
### Owl Callbacks
sub process_owl_jwrite {
    my $body = shift;

    my $j = new Net::Jabber::Message;
    $body =~ s/\n\z//;
    $j->SetMessage(
        to   => $vars{jwrite}{to},
        from => $vars{jwrite}{from},
        type => $vars{jwrite}{type},
        body => $body
    );

    $j->SetThread( $vars{jwrite}{thread} )   if ( $vars{jwrite}{thread} );
    $j->SetSubject( $vars{jwrite}{subject} ) if ( $vars{jwrite}{subject} );

    my $m = j2o( $j, { direction => 'out' } );
    if ( $vars{jwrite}{type} ne 'groupchat') {
        BarnOwl::queue_message($m);
    }

    $j->RemoveFrom(); # Kludge to get around gtalk's random bits after the resource.
    if ($vars{jwrite}{sid} && $conn->sidExists( $vars{jwrite}{sid} )) {
        $conn->getConnectionFromSid($vars{jwrite}{sid})->Send($j);
    }
    else {
        $conn->getConnectionFromJID($vars{jwrite}{from})->Send($j);
    }

    delete $vars{jwrite};
    BarnOwl::message("");   # Kludge to make the ``type your message...'' message go away
}

### XMPP Callbacks

sub process_incoming_chat_message {
    my ( $sid, $j ) = @_;
    if ($j->DefinedBody() || BarnOwl::getvar('jabber:spew') eq 'on') {
        BarnOwl::queue_message( j2o( $j, { direction => 'in',
                                           sid => $sid } ) );
    }
}

sub process_incoming_error_message {
    my ( $sid, $j ) = @_;
    my %jhash = j2hash( $j, { direction => 'in',
                              sid => $sid } );
    $jhash{type} = 'admin';

    BarnOwl::queue_message( BarnOwl::Message->new(%jhash) );
}

sub process_incoming_groupchat_message {
    my ( $sid, $j ) = @_;

    # HACK IN PROGRESS (ignoring delayed messages)
    return if ( $j->DefinedX('jabber:x:delay') && $j->GetX('jabber:x:delay') );
    BarnOwl::queue_message( j2o( $j, { direction => 'in',
                                   sid => $sid } ) );
}

sub process_incoming_headline_message {
    my ( $sid, $j ) = @_;
    BarnOwl::queue_message( j2o( $j, { direction => 'in',
                                   sid => $sid } ) );
}

sub process_incoming_normal_message {
    my ( $sid, $j ) = @_;
    my %jhash = j2hash( $j, { direction => 'in',
                              sid => $sid } );

    # XXX TODO: handle things such as MUC invites here.

    #    if ($j->HasX('http://jabber.org/protocol/muc#user'))
    #    {
    #	my $x = $j->GetX('http://jabber.org/protocol/muc#user');
    #	if ($x->HasChild('invite'))
    #	{
    #	    $props
    #	}
    #    }
    #
    if ($j->DefinedBody() || BarnOwl::getvar('jabber:spew') eq 'on') {
        BarnOwl::queue_message( BarnOwl::Message->new(%jhash) );
    }
}

sub process_muc_presence {
    my ( $sid, $p ) = @_;
    return unless ( $p->HasX('http://jabber.org/protocol/muc#user') );
}


sub process_presence_available {
    my ( $sid, $p ) = @_;
    my $from = $p->GetFrom('jid')->GetJID('base');
    $completion_jids{$from} = 1;
    return unless (BarnOwl::getvar('jabber:show_logins') eq 'on');
    my $to = $p->GetTo();
    my $type = $p->GetType();
    my %props = (
        to => $to,
        from => $p->GetFrom(),
        recipient => $to,
        sender => $from,
        type => 'jabber',
        jtype => $p->GetType(),
        status => $p->GetStatus(),
        show => $p->GetShow(),
        xml => $p->GetXML(),
        direction => 'in');

    if ($type eq '' || $type eq 'available') {
        $props{body} = "$from is now online. ";
        $props{loginout} = 'login';
    }
    else {
        $props{body} = "$from is now offline. ";
        $props{loginout} = 'logout';
    }
    BarnOwl::queue_message(BarnOwl::Message->new(%props));
}

sub process_presence_subscribe {
    my ( $sid, $p ) = @_;
    my $from = $p->GetFrom();
    my $to = $p->GetTo();
    my %props = (
        to => $to,
        from => $from,
        xml => $p->GetXML(),
        type => 'admin',
        adminheader => 'Jabber presence: subscribe',
        direction => 'in');

    $props{body} = "Allow user ($from) to subscribe to your ($to) presence?\n" .
                   "(Answer with the `yes' or `no' commands)";
    $props{yescommand} = BarnOwl::quote('jroster', 'auth', $from, '-a', $to);
    $props{nocommand} = BarnOwl::quote('jroster', 'deauth', $from, '-a', $to);
    $props{question} = "true";
    BarnOwl::queue_message(BarnOwl::Message->new(%props));
}

sub process_presence_unsubscribe {
    my ( $sid, $p ) = @_;
    my $from = $p->GetFrom();
    my $to = $p->GetTo();
    my %props = (
        to => $to,
        from => $from,
        xml => $p->GetXML(),
        type => 'admin',
        adminheader => 'Jabber presence: unsubscribe',
        direction => 'in');

    $props{body} = "The user ($from) has been unsubscribed from your ($to) presence.\n";
    BarnOwl::queue_message(BarnOwl::Message->new(%props));

    # Find a connection to reply with.
    foreach my $jid ($conn->getJIDs()) {
	my $cJID = new Net::Jabber::JID;
	$cJID->SetJID($jid);
	if ($to eq $cJID->GetJID('base') ||
            $to eq $cJID->GetJID('full')) {
	    my $reply = $p->Reply(type=>"unsubscribed");
	    $conn->getConnectionFromJID($jid)->Send($reply);
	    return;
	}
    }
}

sub process_presence_subscribed {
    my ( $sid, $p ) = @_;
    queue_admin_msg("ignoring:".$p->GetXML()) if BarnOwl::getvar('jabber:spew') eq 'on';
    # RFC 3921 says we should respond to this with a "subscribe"
    # but this causes a flood of sub/sub'd presence packets with
    # some servers, so we won't. We may want to detect this condition
    # later, and have per-server settings.
    return;
}

sub process_presence_unsubscribed {
    my ( $sid, $p ) = @_;
    queue_admin_msg("ignoring:".$p->GetXML()) if BarnOwl::getvar('jabber:spew') eq 'on';
    # RFC 3921 says we should respond to this with a "subscribe"
    # but this causes a flood of unsub/unsub'd presence packets with
    # some servers, so we won't. We may want to detect this condition
    # later, and have per-server settings.
    return;
}

sub process_presence_error {
    my ( $sid, $p ) = @_;
    my $code = $p->GetErrorCode();
    my $error = $p->GetError();
    BarnOwl::error("Jabber: $code $error");
}


### Helper functions

sub j2hash {
    my $j   = shift;
    my %props = (type => 'jabber',
                 dir  => 'none',
                 %{$_[0]});

    my $dir = $props{direction};

    my $jtype = $props{jtype} = $j->GetType();
    my $from = $j->GetFrom('jid');
    my $to   = $j->GetTo('jid');

    $props{from} = $from->GetJID('full');
    $props{to}   = $to->GetJID('full');

    $props{recipient}  = $to->GetJID('base');
    $props{sender}     = $from->GetJID('base');
    $props{subject}    = $j->GetSubject() if ( $j->DefinedSubject() );
    $props{thread}     = $j->GetThread() if ( $j->DefinedThread() );
    if ( $j->DefinedBody() ) {
        $props{body}   = $j->GetBody();
        $props{body}  =~ s/\xEF\xBB\xBF//g; # Strip stray Byte-Order-Marks.
    }
    $props{error}      = $j->GetError() if ( $j->DefinedError() );
    $props{error_code} = $j->GetErrorCode() if ( $j->DefinedErrorCode() );
    $props{xml}        = $j->GetXML();

    if ( $jtype eq 'groupchat' ) {
        my $nick = $props{nick} = $from->GetResource();
        my $room = $props{room} = $from->GetJID('base');
        $completion_jids{$room} = 1;

        my $muc;
        if ($dir eq 'in') {
            my $connection = $conn->getConnectionFromSid($props{sid});
            $muc = $connection->FindMUC(jid => $from);
        } else {
            my $connection = $conn->getConnectionFromJID($props{from});
            $muc = $connection->FindMUC(jid => $to);
        }
        $props{from} = $muc->GetFullJID($from) || $props{from};
        $props{sender} = $nick || $room;
        $props{recipient} = $room;

        if ( $props{subject} && !$props{body} ) {
            $props{body} =
              '[' . $nick . " has set the topic to: " . $props{subject} . "]";
        }
    }
    elsif ( $jtype eq 'headline' ) {
        ;
    }
    elsif ( $jtype eq 'error' ) {
        $props{body}     = "Error "
          . $props{error_code}
          . " sending to "
          . $props{from} . "\n"
          . $props{error};
    }
    else { # chat, or normal (default)
        $props{private} = 1;

        my $connection;
        if ($dir eq 'in') {
            $connection = $conn->getConnectionFromSid($props{sid});
        }
        else {
            $connection = $conn->getConnectionFromJID($props{from});
        }

        # Check to see if we're doing personals with someone in a muc.
        # If we are, show the full jid because the base jid is the room.
        if ($connection) {
            $props{sender} = $props{from}
              if ($connection->FindMUC(jid => $from));
            $props{recipient} = $props{to}
              if ($connection->FindMUC(jid => $to));
        }

        # Populate completion.
        if ($dir eq 'in') {
            $completion_jids{ $props{sender} }= 1;
        }
        else {
            $completion_jids{ $props{recipient} } = 1;
        }
    }

    return %props;
}

sub j2o {
    return BarnOwl::Message->new( j2hash(@_) );
}

sub queue_admin_msg {
    my $err = shift;
    BarnOwl::admin_message("Jabber", $err);
}

sub getServerFromJID {
    my $jid = shift;
    my $res = new Net::DNS::Resolver;
    my $packet =
      $res->search( '_xmpp-client._tcp.' . $jid->GetServer(), 'srv' );

    if ($packet)    # Got srv record.
    {
        my @answer = $packet->answer;
        return $answer[0]{target}, $answer[0]{port};
    }

    return $jid->GetServer(), 5222;
}

sub defaultJID {
    return ( $conn->getJIDs() )[0] if ( $conn->connected() == 1 );
    return;
}

sub baseJID {
    my $givenJIDStr = shift;
    my $givenJID    = new Net::Jabber::JID;
    $givenJID->SetJID($givenJIDStr);
    return $givenJID->GetJID('base');
}

sub resolveConnectedJID {
    my $givenJIDStr = shift;
    my $loose = shift || 0;
    my $givenJID    = new Net::Jabber::JID;
    $givenJID->SetJID($givenJIDStr);

    # Account fully specified.
    if ( $givenJID->GetResource() ) {
        # Specified account exists
        return $givenJIDStr if ($conn->jidExists($givenJIDStr) );
        return resolveConnectedJID($givenJID->GetJID('base')) if $loose;
        die("Invalid account: $givenJIDStr\n");
    }

    # Disambiguate.
    else {
        my $JIDMatchingJID = "";
        my $strMatchingJID = "";
        my $JIDMatches = "";
        my $strMatches = "";
        my $JIDAmbiguous = 0;
        my $strAmbiguous = 0;

        foreach my $jid ( $conn->getJIDs() ) {
            my $cJID = new Net::Jabber::JID;
            $cJID->SetJID($jid);
            if ( $givenJIDStr eq $cJID->GetJID('base') ) {
                $JIDAmbiguous = 1 if ( $JIDMatchingJID ne "" );
                $JIDMatchingJID = $jid;
                $JIDMatches .= "\t$jid\n";
            }
            if ( $cJID->GetJID('base') =~ /$givenJIDStr/ ) {
                $strAmbiguous = 1 if ( $strMatchingJID ne "" );
                $strMatchingJID = $jid;
                $strMatches .= "\t$jid\n";
            }
        }

        # Need further disambiguation.
        if ($JIDAmbiguous) {
            my $errStr =
                "Ambiguous account reference. Please specify a resource.\n";
            die($errStr.$JIDMatches);
        }

        # It's this one.
        elsif ($JIDMatchingJID ne "") {
            return $JIDMatchingJID;
        }

        # Further resolution by substring.
        elsif ($strAmbiguous) {
            my $errStr =
                "Ambiguous account reference. Please be more specific.\n";
            die($errStr.$strMatches);
        }

        # It's this one, by substring.
        elsif ($strMatchingJID ne "") {
            return $strMatchingJID;
        }

        # Not one of ours.
        else {
            die("Invalid account: $givenJIDStr\n");
        }

    }
    return "";
}

sub resolveDestJID {
    my ($to, $from) = @_;
    my $jid = Net::Jabber::JID->new($to);

    my $roster = $conn->getRosterFromJID($from);
    my @jids = $roster->jids('all');
    for my $j (@jids) {
        if(($roster->query($j, 'name') || $j->GetUserID()) eq $to) {
            return $j->GetJID('full');
        } elsif($j->GetJID('base') eq baseJID($to)) {
            return $jid->GetJID('full');
        }
    }

    # If we found nothing being clever, check to see if our input was
    # sane enough to look like a jid with a UserID.
    return $jid->GetJID('full') if $jid->GetUserID();
    return undef;
}

sub resolveType {
    my $to = shift;
    my $from = shift;
    return unless $from;
    my @mucs = $conn->getConnectionFromJID($from)->MUCs;
    if(grep {$_->BaseJID eq $to } @mucs) {
        return 'groupchat';
    } else {
        return 'chat';
    }
}

sub guess_jwrite {
    # Heuristically guess what jids a jwrite was meant to be going to/from
    my ($from, $to) = (@_);
    my ($from_jid, $to_jid);
    my @matches;
    if($from) {
        $from_jid = resolveConnectedJID($from, 1);
        die("Unable to resolve account $from\n") unless $from_jid;
        $to_jid = resolveDestJID($to, $from_jid);
        push @matches, [$from_jid, $to_jid] if $to_jid;
    } else {
        for my $f ($conn->getJIDs) {
            $to_jid = resolveDestJID($to, $f);
            if(defined($to_jid)) {
                push @matches, [$f, $to_jid];
            }
        }
        if($to =~ /@/) {
            push @matches, [$_, $to]
               for ($conn->getJIDs);
        }
    }

    for my $m (@matches) {
        my $type = resolveType($m->[1], $m->[0]);
        push @$m, $type;
    }

    return @matches;
}

################################################################################
### Completion

sub complete_user_or_muc { return keys %completion_jids; }
sub complete_account { return $conn->getJIDs(); }

sub complete_jwrite {
    my $ctx = shift;
    return complete_flags($ctx,
                          [qw(-t -i -s)],
                          {
                              "-a" => \&complete_account,
                          },
                          \&complete_user_or_muc
        );
}

sub complete_jabberlogout {
    my $ctx = shift;
    if($ctx->word == 1) {
        return ("-A", complete_account() );
    } else {
        return ();
    }
}

BarnOwl::Completion::register_completer(jwrite => sub { BarnOwl::Module::Jabber::complete_jwrite(@_) });
BarnOwl::Completion::register_completer(jabberlogout => sub { BarnOwl::Module::Jabber::complete_jabberlogout(@_) });

1;
