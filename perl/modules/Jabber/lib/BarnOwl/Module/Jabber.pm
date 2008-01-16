use strict;
use warnings;

package BarnOwl::Module::Jabber;

=head1 NAME

BarnOwl::Module::Jabber

=head1 DESCRIPTION

This module implements Jabber support for barnowl.

=cut

use BarnOwl;
use BarnOwl::Hooks;
use BarnOwl::Message::Jabber;
use BarnOwl::Module::Jabber::Connection;
use BarnOwl::Module::Jabber::ConnectionManager;

use Authen::SASL qw(Perl);
use Net::Jabber;
use Net::Jabber::MUC;
use Net::DNS;
use Getopt::Long;

our $VERSION = 0.1;

BEGIN {
    if(eval {require IO::Socket::SSL;}) {
        if($IO::Socket::SSL::VERSION eq "0.97") {
            BarnOwl::error("You are using IO::Socket:SSL 0.97, which \n" .
                           "contains bugs causing it not to work with barnowl's\n" .
                           "jabber support. We recommend updating to the latest\n" .
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

sub onStart {
    if ( *BarnOwl::queue_message{CODE} ) {
        register_owl_commands();
        register_keybindings();
        register_filters();
        $BarnOwl::Hooks::mainLoop->add(\&onMainLoop);
        $BarnOwl::Hooks::getBuddyList->add(\&onGetBuddyList);
        $vars{show} = '';
	BarnOwl::new_variable_bool("jabber:show_offline_buddies",
				   { default => 1,
				     summary => 'Show offline or pending buddies.'});
	BarnOwl::new_variable_int("jabber:auto_away_timeout",
				  { default => 5,
				    summary => 'After minutes idle, auto away.',
				  });
	BarnOwl::new_variable_int("jabber:auto_xa_timeout",
				  { default => 15,
				    summary => 'After minutes idle, auto extended away.'
				});
    } else {
        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support BarnOwl::error. So just
        # give up silently.
    }
}

$BarnOwl::Hooks::startup->add(\&onStart);

sub onMainLoop {
    return if ( !$conn->connected() );

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
    } elsif ($idletime == 0 && $vars{show} ne '') {
        $vars{show} = '';
        $vars{status} = '';
        $vars{status_changed} = 1;
    }

    foreach my $jid ( $conn->getJIDs() ) {
        my $client = $conn->getConnectionFromJID($jid);

        unless($client) {
            $conn->removeConnection($jid);
            BarnOwl::error("Connection for $jid undefined -- error in reload?");
        }

        my $status = $client->Process(0);
        if ( !defined($status) ) {
            BarnOwl::error("Jabber account $jid disconnected!");
            do_logout($jid);
        }
        if ($::shutdown) {
            do_logout($jid);
            return;
        }
        if ($vars{status_changed}) {
            my $p = new Net::Jabber::Presence;
            $p->SetShow($vars{show}) if $vars{show};
            $p->SetStatus($vars{status}) if $vars{status};
            $client->Send($p);
        }
    }
}

sub blist_listBuddy {
    my $roster = shift;
    my $buddy  = shift;
    my $blistStr .= "    ";
    my %jq  = $roster->query($buddy);
    my $res = $roster->resource($buddy);

    my $name = $jq{name} || $buddy->GetUserID();

    $blistStr .= sprintf '%-15s %s', $name, $buddy->GetJID();

    if ($res) {
        my %rq = $roster->resourceQuery( $buddy, $res );
        $blistStr .= " [" . ( $rq{show} ? $rq{show} : 'online' ) . "]";
        $blistStr .= " " . $rq{status} if $rq{status};
        $blistStr = BarnOwl::Style::boldify($blistStr);
    }
    else {
        return '' if (BarnOwl::getvar('jabber:show_offline_buddies') eq 'off');
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

sub getSingleBuddyList {
    my $jid = shift;
    $jid = resolveConnectedJID($jid);
    return "" unless $jid;
    my $blist = "";
    my $roster = $conn->getRosterFromJID($jid);
    if ($roster) {
        $blist .= "\n" . BarnOwl::Style::boldify("Jabber Roster for $jid\n");

        foreach my $group ( $roster->groups() ) {
            $blist .= "  Group: $group\n";
            my @buddies = $roster->jids( 'group', $group );
            foreach my $buddy ( @buddies ) {
                $blist .= blist_listBuddy( $roster, $buddy );
            }
        }

        my @unsorted = $roster->jids('nogroup');
        if (@unsorted) {
            $blist .= "  [unsorted]\n";
            foreach my $buddy (@unsorted) {
                $blist .= blist_listBuddy( $roster, $buddy );
            }
        }
    }
    return $blist;
}

sub onGetBuddyList {
    my $blist = "";
    foreach my $jid ($conn->getJIDs()) {
        $blist .= getSingleBuddyList($jid);
    }
    return $blist;
}

################################################################################
### Owl Commands
sub register_owl_commands() {
    BarnOwl::new_command(
        jabberlogin => \&cmd_login,
        {
            summary => "Log into jabber",
            usage   => "jabberlogin JID [PASSWORD]"
        }
    );
    BarnOwl::new_command(
        jabberlogout => \&cmd_logout,
        { summary => "Log out of jabber" }
    );
    BarnOwl::new_command(
        jwrite => \&cmd_jwrite,
        {
            summary => "Send a Jabber Message",
            usage   => "jwrite JID [-t thread] [-s subject]"
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
            description => "jmuc sends jabber commands related to muc.\n\n"
              . "The following commands are available\n\n"
              . "join <muc>  Join a muc.\n\n"
              . "part <muc>  Part a muc.\n"
              . "            The muc is taken from the current message if not supplied.\n\n"
              . "invite <jid> <muc>\n"
              . "            Invite <jid> to <muc>.\n"
              . "            The muc is taken from the current message if not supplied.\n\n"
              . "configure <muc>\n"
              . "            Configures a MUC.\n"
              . "            Necessary to initalize a new MUC.\n"
              . "            At present, only the default configuration is supported.\n"
              . "            The muc is taken from the current message if not supplied.\n\n"
              . "presence <muc>\n"
              . "            Shows the roster for <muc>.\n"
              . "            The muc is taken from the current message if not supplied.\n\n"
              . "presence -a\n"
              . "            Shows rosters for all MUCs you're participating in.\n\n",
            usage => "jmuc COMMAND ARGS"
        }
    );
    BarnOwl::new_command(
        jroster => \&cmd_jroster,
        {
            summary     => "Jabber Roster related commands.",
            description => "jroster sends jabber commands related to rosters.\n\n"
              . "The following commands are available\n\n"
              . "sub <jid>     Subscribe to <jid>'s presence. (implicit add)\n\n"
              . "add <jid>     Adds <jid> to your roster.\n\n"
              . "unsub <jid>   Unsubscribe from <jid>'s presence.\n\n"
              . "remove <jid>  Removes <jid> to your roster. (implicit unsub)\n\n"
              . "auth <jid>    Authorizes <jid> to subscribe to your presence.\n\n"
              . "deauth <jid>  De-authorizes <jid>'s subscription to your presence.\n\n"
              . "The following arguments are supported for all commands\n\n"
              . "-a <jid>      Specify which account to make the roster changes on.\n"
              . "              Required if you're signed into more than one account.\n\n"
              . "The following arguments only work with the add and sub commands.\n\n"
              . "-g <group>    Add <jid> to group <group>.\n"
              . "              May be specified more than once, will not remove <jid> from any groups.\n\n"
              . "-p            Purge. Removes <jid> from all groups.\n"
              . "              May be combined with -g completely alter <jid>'s groups.\n\n"
              . "-n <name>     Sets <name> as <jid>'s short name.\n\n"
              . "Note: Unless -n is used, you can specify multiple <jid> arguments.\n",
            usage       => "jroster COMMAND ARGS"
        }
    );
}

sub register_keybindings {
    BarnOwl::bindkey("recv j command start-command jwrite ");
}

sub register_filters {
    BarnOwl::filter('jabber type ^jabber$');
}

sub cmd_login {
    my $cmd = shift;
    my $jid = new Net::Jabber::JID;
    $jid->SetJID(shift);
    my $password = '';
    $password = shift if @_;

    my $uid           = $jid->GetUserID();
    my $componentname = $jid->GetServer();
    my $resource      = $jid->GetResource() || 'owl';
    $jid->SetResource($resource);
    my $jidStr = $jid->GetJID('full');

    if ( !$uid || !$componentname ) {
        BarnOwl::error("usage: $cmd JID");
        return;
    }

    if ( $conn->jidExists($jidStr) ) {
        BarnOwl::error("Already logged in as $jidStr.");
        return;
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
        BarnOwl::error("Got password but have no jid!");
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
            BarnOwl::error("We failed to connect");
        } else {
            my @result = $client->AuthSend( %{ $vars{jlogin_authhash} } );

            if ( !@result || $result[0] ne 'ok' ) {
                if ( !$vars{jlogin_havepass} && ( !@result || $result[0] eq '401' || $result[0] eq 'error') ) {
                    $vars{jlogin_havepass} = 1;
                    $conn->removeConnection($jidStr);
                    BarnOwl::start_password( "Password for $jidStr: ", \&do_login );
                    return "";
                }
                $conn->removeConnection($jidStr);
                BarnOwl::error( "Error in connect: " . join( " ", @result ) );
            } else {
                $conn->getRosterFromJID($jidStr)->fetch();
                $client->PresenceSend( priority => 1 );
		my $fullJid = $client->{SESSION}->{FULLJID} || $jidStr;
		$conn->renameConnection($jidStr, $fullJid);
                queue_admin_msg("Connected to jabber as $fullJid");
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
            if ( $_[1] eq '-a' )    #All accounts.
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
        BarnOwl::error("You are not logged in to Jabber.");
        return;
    }
    BarnOwl::popless_ztext( onGetBuddyList() );
}

sub cmd_jwrite {
    if ( !$conn->connected() ) {
        BarnOwl::error("You are not logged in to Jabber.");
        return;
    }

    my $jwrite_to      = "";
    my $jwrite_from    = "";
    my $jwrite_sid     = "";
    my $jwrite_thread  = "";
    my $jwrite_subject = "";
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
        'id=s'     =>  \$jwrite_sid,
    );
    $jwrite_type = 'groupchat' if $gc;

    if ( scalar @ARGV != 1 ) {
        BarnOwl::error(
            "Usage: jwrite JID [-t thread] [-s 'subject'] [-a account]");
        return;
    }
    else {
        $to = shift @ARGV;
    }

    my @candidates = guess_jwrite($from, $to);

    unless(scalar @candidates) {
        die("Unable to resolve JID $to");
    }

    @candidates = grep {defined $_->[0]} @candidates;

    unless(scalar @candidates) {
        if(!$from) {
            die("You must specify an account with -a");
        } else {
            die("Unable to resolve account $from");
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

    if(scalar @candidates > 1) {
        BarnOwl::message(
            "Warning: Guessing account and/or destination JID"
           );
    } else  {
        BarnOwl::message(
            "Type your message below.  End with a dot on a line by itself.  ^C will quit."
           );
    }

    my $cmd = "jwrite $jwrite_to -a $jwrite_from";
    $cmd .= " -t $jwrite_thread" if $jwrite_thread;
    $cmd .= " -s $jwrite_subject" if $jwrite_subject;
    BarnOwl::start_edit_win( $cmd, \&process_owl_jwrite );
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
        BarnOwl::error("jmuc: Unknown command: $cmd");
        return;
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
        $getopt->configure('pass_through');
        $getopt->getoptions( 'account=s' => \$jid );
        $jid ||= defaultJID();
        if ($jid) {
            $jid = resolveConnectedJID($jid);
            return unless $jid;
        }
        else {
            BarnOwl::error('You must specify an account with -a {jid}');
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
      or die("Usage: jmuc join MUC [-p password] [-a account]");

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
    return;
}

sub jmuc_part {
    my ( $jid, $muc, @args ) = @_;

    $muc = shift @args if scalar @args;
    die("Usage: jmuc part MUC [-a account]") unless $muc;

    if($conn->getConnectionFromJID($jid)->MUCLeave(JID => $muc)) {
        queue_admin_msg("$jid has left $muc.");
    } else {
        die("Error: Not joined to $muc");
    }
}

sub jmuc_invite {
    my ( $jid, $muc, @args ) = @_;

    my $invite_jid = shift @args;
    $muc = shift @args if scalar @args;

    die('Usage: jmuc invite JID [muc] [-a account]')
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
    die("Usage: jmuc configure [muc]") unless $muc;
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
    die("Usage: jmuc presence MUC") unless $muc;

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
        die("No such muc: $muc") unless $m;
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
        BarnOwl::error("jroster: Unknown command: $cmd");
        return;
    }

    {
        local @ARGV = @_;
        my $jid;
        my $name;
        my @groups;
        my $purgeGroups;
        my $getopt = Getopt::Long::Parser->new;
        $getopt->configure('pass_through');
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
            BarnOwl::error('You must specify an account with -a {jid}');
        }
        return $func->( $jid, $name, \@groups, $purgeGroups,  @ARGV );
    }
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
    BarnOwl::queue_message( j2o( $j, { direction => 'in',
                                   sid => $sid } ) );
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
    BarnOwl::queue_message( BarnOwl::Message->new(%jhash) );
}

sub process_muc_presence {
    my ( $sid, $p ) = @_;
    return unless ( $p->HasX('http://jabber.org/protocol/muc#user') );
}


sub process_presence_available {
    my ( $sid, $p ) = @_;
    my $from = $p->GetFrom();
    my $to = $p->GetTo();
    my $type = $p->GetType();
    my %props = (
        to => $to,
        from => $from,
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
    $props{replysendercmd} = $props{replycmd} = "jwrite $from -i $sid";
    if(BarnOwl::getvar('debug') eq 'on') {
        BarnOwl::queue_message(BarnOwl::Message->new(%props));
    }
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
    $props{yescommand} = "jroster auth $from -a $to";
    $props{nocommand} = "jroster deauth $from -a $to";
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
    queue_admin_msg("ignoring:".$p->GetXML());
    # RFC 3921 says we should respond to this with a "subscribe"
    # but this causes a flood of sub/sub'd presence packets with
    # some servers, so we won't. We may want to detect this condition
    # later, and have per-server settings.
    return;
}

sub process_presence_unsubscribed {
    my ( $sid, $p ) = @_;
    queue_admin_msg("ignoring:".$p->GetXML());
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
    my %initProps = %{ shift() };

    my $dir = 'none';
    my %props = ( type => 'jabber' );

    foreach my $k (keys %initProps) {
        $dir = $initProps{$k} if ($k eq 'direction');
        $props{$k} = $initProps{$k};
    }

    my $jtype = $props{jtype} = $j->GetType();
    my $from = $j->GetFrom('jid');
    my $to   = $j->GetTo('jid');

    $props{from} = $from->GetJID('full');
    $props{to}   = $to->GetJID('full');

    $props{recipient}  = $to->GetJID('base');
    $props{sender}     = $from->GetJID('base');
    $props{subject}    = $j->GetSubject() if ( $j->DefinedSubject() );
    $props{thread}     = $j->GetThread() if ( $j->DefinedThread() );
    $props{body}       = $j->GetBody() if ( $j->DefinedBody() );
    $props{error}      = $j->GetError() if ( $j->DefinedError() );
    $props{error_code} = $j->GetErrorCode() if ( $j->DefinedErrorCode() );
    $props{xml}        = $j->GetXML();

    if ( $jtype eq 'chat' ) {
        $props{replycmd} =
          "jwrite " . ( ( $dir eq 'in' ) ? $props{from} : $props{to} );
        $props{replycmd} .=
          " -a " . ( ( $dir eq 'out' ) ? $props{from} : $props{to} );
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
    }
    elsif ( $jtype eq 'groupchat' ) {
        my $nick = $props{nick} = $from->GetResource();
        my $room = $props{room} = $from->GetJID('base');
        $props{replycmd} = "jwrite $room";
        $props{replycmd} .=
          " -a " . ( ( $dir eq 'out' ) ? $props{from} : $props{to} );
	if ($props{subject}) {
	  $props{replycmd} .= " -s " . $props{subject}
	}

        if ($dir eq 'out') {
            $props{replysendercmd} = "jwrite ".$props{to}." -a ".$props{from};
        }
        else {
            $props{replysendercmd} = "jwrite ".$props{from}." -a ".$props{to};
        }

        $props{sender} = $nick || $room;
        $props{recipient} = $room;

        if ( $props{subject} && !$props{body} ) {
            $props{body} =
              '[' . $nick . " has set the topic to: " . $props{subject} . "]";
        }
    }
    elsif ( $jtype eq 'normal' ) {
        $props{replycmd}  = "";
        $props{private} = 1;
    }
    elsif ( $jtype eq 'headline' ) {
        $props{replycmd} = "";
    }
    elsif ( $jtype eq 'error' ) {
        $props{replycmd} = "";
        $props{body}     = "Error "
          . $props{error_code}
          . " sending to "
          . $props{from} . "\n"
          . $props{error};
    }

    $props{replysendercmd} = $props{replycmd} unless $props{replysendercmd};
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
    my $givenJID    = new Net::Jabber::JID;
    $givenJID->SetJID($givenJIDStr);

    # Account fully specified.
    if ( $givenJID->GetResource() ) {
        # Specified account exists
        return $givenJIDStr if ($conn->jidExists($givenJIDStr) );
        die("Invalid account: $givenJIDStr");
    }

    # Disambiguate.
    else {
        my $matchingJID = "";
        my $errStr =
          "Ambiguous account reference. Please specify a resource.\n";
        my $ambiguous = 0;

        foreach my $jid ( $conn->getJIDs() ) {
            my $cJID = new Net::Jabber::JID;
            $cJID->SetJID($jid);
            if ( $givenJIDStr eq $cJID->GetJID('base') ) {
                $ambiguous = 1 if ( $matchingJID ne "" );
                $matchingJID = $jid;
                $errStr .= "\t$jid\n";
            }
        }

        # Need further disambiguation.
        if ($ambiguous) {
            die($errStr);
        }

        # Not one of ours.
        elsif ( $matchingJID eq "" ) {
            die("Invalid account: $givenJIDStr");
        }

        # It's this one.
        else {
            return $matchingJID;
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
        $from_jid = resolveConnectedJID($from);
        die("Unable to resolve account $from") unless $from_jid;
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

1;
