# -*- mode: cperl; cperl-indent-level: 4; indent-tabs-mode: nil -*-
package owl_jabber;
use warnings;
use strict;

use Authen::SASL qw(Perl);
use Net::Jabber;
use Net::DNS;
use Getopt::Long;

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
#  * Current behavior is auto-accept (default for Net::Jabber)
#
################################################################################


################################################################################
################################################################################
package owl_jabber::ConnectionManager;
sub new {
    my $class = shift;
    return bless { }, $class;
}

sub addConnection {
    my $self = shift;
    my $jidStr = shift;

    my %args = ();
    if(owl::getvar('debug') eq 'on') {
        $args{debuglevel} = 1;
        $args{debugfile} = 'jabber.log';
    }
    my $client = Net::Jabber::Client->new(%args);

    $self->{Client}->{$jidStr} = $client;
    $self->{Roster}->{$jidStr} = $client->Roster();
    return $client;
}

sub removeConnection {
    my $self = shift;
    my $jidStr = shift;
    return 0 unless exists $self->{Client}->{$jidStr};

    $self->{Client}->{$jidStr}->Disconnect();
    delete $self->{Roster}->{$jidStr};
    delete $self->{Client}->{$jidStr};

    return 1;
}

sub connected {
    my $self = shift;
    return scalar keys %{ $self->{Client} };
}

sub getJids {
    my $self = shift;
    return keys %{ $self->{Client} };
}

sub jidExists {
    my $self = shift;
    my $jidStr = shift;
    return exists $self->{Client}->{$jidStr};
}

sub sidExists {
    my $self = shift;
    my $sid = shift || "";
    foreach my $j ( keys %{ $self->{Client} } ) {
        return 1 if ($self->{Client}->{$j}->{SESSION}->{id} eq $sid);
    }
    return 0;
}

sub getConnectionFromSid {
    my $self = shift;
    my $sid = shift;
    foreach my $c (values %{ $self->{Client} }) {
        return $c if $c->{SESSION}->{id} eq $sid;
    }
    return undef;
}

sub getConnectionFromJidStr {
    my $self = shift;
    my $jidStr = shift;
    return $self->{Client}->{$jidStr};
}

sub getRosterFromSid {
    my $self = shift;
    my $sid = shift;
    foreach my $j ( $self->getJids ) {
        return $self->{Roster}->{$j}
          if $self->{Client}->{$j}->{SESSION}->{id} eq $sid;
    }
    return undef;
}

sub getRosterFromJidStr {
    my $self = shift;
    my $jidStr = shift;
    return $self->{Roster}->{$jidStr};
    return undef;
}
################################################################################

package owl_jabber;

our $conn = new owl_jabber::ConnectionManager unless $conn;;
our %vars;

sub onStart {
    if ( eval { \&owl::queue_message } ) {
        register_owl_commands();
        push @::onMainLoop,     sub { owl_jabber::onMainLoop(@_) };
        push @::onGetBuddyList, sub { owl_jabber::onGetBuddyList(@_) };
    } else {
        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support owl::error. So just
        # give up silently.
    }
}

push @::onStartSubs, sub { owl_jabber::onStart(@_) };

sub onMainLoop {
    return if ( !$conn->connected() );

    foreach my $jid ( $conn->getJids() ) {
        my $client = $conn->getConnectionFromJidStr($jid);

        my $status = $client->Process(0);
        if ( !defined($status) ) {
            owl::error("Jabber account $jid disconnected!");
            do_logout($jid);
        }
        if ($::shutdown) {
            do_logout($jid);
            return;
        }
    }
}

sub blist_listBuddy {
    my $roster = shift;
    my $buddy  = shift;
    my $blistStr .= "    ";
    my %jq  = $roster->query($buddy);
    my $res = $roster->resource($buddy);

    $blistStr .= $jq{name} ? $jq{name} . "\t(" .$buddy->GetJID() . ')' : $buddy->GetJID();

    if ($res) {
        my %rq = $roster->resourceQuery( $buddy, $res );
        $blistStr .= " [" . ( $rq{show} ? $rq{show} : 'online' ) . "]";
        $blistStr .= " " . $rq{status} if $rq{status};
        $blistStr = boldify($blistStr);
    }
    else {
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
    $jid = resolveJID($jid);
    return "" unless $jid;
    my $blist = "";
    my $roster = $conn->getRosterFromJidStr($jid);
    if ($roster) {
        $blist .= "\n" . boldify("Jabber Roster for $jid\n");

        foreach my $group ( $roster->groups() ) {
            $blist .= "  Group: $group\n";
            foreach my $buddy ( $roster->jids( 'group', $group ) ) {
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
    foreach my $jid ($conn->getJids()) {
        $blist .= getSingleBuddyList($jid);
    }
    return $blist;
}

################################################################################
### Owl Commands
sub register_owl_commands() {
    owl::new_command(
        jabberlogin => \&cmd_login,
        { summary => "Log into jabber", }
    );
    owl::new_command(
        jabberlogout => \&cmd_logout,
        { summary => "Log out of jabber" }
    );
    owl::new_command(
        jwrite => \&cmd_jwrite,
        {
            summary => "Send a Jabber Message",
            usage   => "jwrite JID [-g] [-t thread] [-s subject]"
        }
    );
    owl::new_command(
        jlist => \&cmd_jlist,
        {
            summary => "Show your Jabber roster.",
            usage   => "jlist"
        }
    );
    owl::new_command(
        jmuc => \&cmd_jmuc,
        {
            summary     => "Jabber MUC related commands.",
            description => "jmuc sends jabber commands related to muc.\n\n"
              . "The following commands are available\n\n"
              . "join {muc}  Join a muc.\n\n"
              . "part [muc]  Part a muc.\n"
              . "            The muc is taken from the current message if not supplied.\n\n"
              . "invite {jid} [muc]\n"
              . "            Invite {jid} to [muc].\n"
              . "            The muc is taken from the current message if not supplied.\n\n"
              . "configure [muc]\n"
              . "            Configure [muc].\n"
              . "            Necessary to initalize a new MUC",
            usage => "jmuc {command} {args}"
        }
    );
    owl::new_command(
        jroster => \&cmd_jroster,
        {
            summary     => "Jabber Roster related commands.",
	    description => "jroster sends jabber commands related to rosters.\n\n",
            usage       => "jroster {command} {args}"
        }
    );
}

sub cmd_login {
    my $cmd = shift;
    my $jid = new Net::XMPP::JID;
    $jid->SetJID(shift);

    my $uid           = $jid->GetUserID();
    my $componentname = $jid->GetServer();
    my $resource      = $jid->GetResource() || 'owl';
    $jid->SetResource($resource);
    my $jidStr = $jid->GetJID('full');

    if ( !$uid || !$componentname ) {
        owl::error("usage: $cmd {jid}");
        return;
    }

    if ( $conn->jidExists($jidStr) ) {
        owl::error("Already logged in as $jidStr.");
        return;
    }

    my ( $server, $port ) = getServerFromJID($jid);

    $vars{jlogin_jid} = $jidStr;
    $vars{jlogin_havepass} = 0;
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

    return do_login('');
}

sub do_login {
    $vars{jlogin_password} = shift;
    $vars{jlogin_authhash}->{password} = sub { return $vars{jlogin_password} || '' };
    my $jidStr = $vars{jlogin_jid};
    if ( !$jidStr && $vars{jlogin_havepass}) {
        owl::error("Got password but have no jid!");
    }
    else
    {
        my $client = $conn->addConnection($jidStr);

        #XXX Todo: Add more callbacks.
        # * MUC presence handlers
        $client->SetMessageCallBacks(
            chat      => sub { owl_jabber::process_incoming_chat_message(@_) },
            error     => sub { owl_jabber::process_incoming_error_message(@_) },
            groupchat => sub { owl_jabber::process_incoming_groupchat_message(@_) },
            headline  => sub { owl_jabber::process_incoming_headline_message(@_) },
            normal    => sub { owl_jabber::process_incoming_normal_message(@_) }
        );
        $client->SetPresenceCallBacks(
#            available    => sub { owl_jabber::process_presence_available(@_) },
#            unavailable  => sub { owl_jabber::process_presence_available(@_) },
            subscribe    => sub { owl_jabber::process_presence_subscribe(@_) },
            subscribed   => sub { owl_jabber::process_presence_subscribed(@_) },
            unsubscribe  => sub { owl_jabber::process_presence_unsubscribe(@_) },
            unsubscribed => sub { owl_jabber::process_presence_unsubscribed(@_) },
            error        => sub { owl_jabber::process_presence_error(@_) });

        my $status = $client->Connect( %{ $vars{jlogin_connhash} } );
        if ( !$status ) {
            $conn->removeConnection($jidStr);
            owl::error("We failed to connect");
        } else {
            my @result = $client->AuthSend( %{ $vars{jlogin_authhash} } );

            if ( $#result == -1 || $result[0] ne 'ok' ) {
                if ( !$vars{jlogin_havepass} && ( $#result == -1 || $result[0] eq '401' ) ) {
                    $vars{jlogin_havepass} = 1;
                    $conn->removeConnection($jidStr);
                    owl::start_password( "Password for $jidStr: ", \&do_login );
                    return "";
                }
                $conn->removeConnection($jidStr);
                owl::error( "Error in connect: " . join( " ", @result ) );
            } else {
                $conn->getRosterFromJidStr($jidStr)->fetch();
                $client->PresenceSend( priority => 1 );
                queue_admin_msg("Connected to jabber as $jidStr");
            }
        }

    }
    delete $vars{jlogin_jid};
    $vars{jlogin_password} =~ tr/\0-\377/x/;
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
            foreach my $jid ( $conn->getJids() ) {
                $errStr .= "\t$jid\n";
            }
            queue_admin_msg($errStr);
        }
        # Logged into multiple accounts, account specified.
        else {
            if ( $_[1] eq '-a' )    #All accounts.
            {
                foreach my $jid ( $conn->getJids() ) {
                    do_logout($jid);
                }
            }
            else                    #One account.
            {
                my $jid = resolveJID( $_[1] );
                do_logout($jid) if ( $jid ne '' );
            }
        }
    }
    else                            # Only one account logged in.
    {
        do_logout( ( $conn->getJids() )[0] );
    }
    return "";
}

sub cmd_jlist {
    if ( !( scalar $conn->getJids() ) ) {
        owl::error("You are not logged in to Jabber.");
        return;
    }
    owl::popless_ztext( onGetBuddyList() );
}

sub cmd_jwrite {
    if ( !$conn->connected() ) {
        owl::error("You are not logged in to Jabber.");
        return;
    }

    my $jwrite_to      = "";
    my $jwrite_from    = "";
    my $jwrite_sid     = "";
    my $jwrite_thread  = "";
    my $jwrite_subject = "";
    my $jwrite_type    = "chat";

    my @args = @_;
    shift;
    local @ARGV = @_;
    my $gc;
    GetOptions(
        'thread=s'  => \$jwrite_thread,
        'subject=s' => \$jwrite_subject,
        'account=s' => \$jwrite_from,
        'id=s'     =>  \$jwrite_sid,
        'groupchat' => \$gc
    );
    $jwrite_type = 'groupchat' if $gc;

    if ( scalar @ARGV != 1 ) {
        owl::error(
            "Usage: jwrite JID [-g] [-t thread] [-s 'subject'] [-a account]");
        return;
    }
    else {
        $jwrite_to = shift @ARGV;
    }

    if ( !$jwrite_from ) {
        if ( $conn->connected() == 1 ) {
            $jwrite_from = ( $conn->getJids() )[0];
        }
        else {
            owl::error("Please specify an account with -a {jid}");
            return;
        }
    }
    else {
        $jwrite_from = resolveJID($jwrite_from);
        return unless $jwrite_from;
    }

    $vars{jwrite} = {
        to      => $jwrite_to,
        from    => $jwrite_from,
        sid     => $jwrite_sid,
        subject => $jwrite_subject,
        thread  => $jwrite_thread,
        type    => $jwrite_type
    };

    owl::message(
"Type your message below.  End with a dot on a line by itself.  ^C will quit."
    );
    owl::start_edit_win( join( ' ', @args ), \&process_owl_jwrite );
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
        configure => \&jmuc_configure
    );
    my $func = $jmuc_commands{$cmd};
    if ( !$func ) {
        owl::error("jmuc: Unknown command: $cmd");
        return;
    }

    {
        local @ARGV = @_;
        my $jid;
        my $muc;
        my $m = owl::getcurmsg();
        if ( $m->is_jabber && $m->{jtype} eq 'groupchat' ) {
            $muc = $m->{room};
            $jid = $m->{to};
        }

        my $getopt = Getopt::Long::Parser->new;
        $getopt->configure('pass_through');
        $getopt->getoptions( 'account=s' => \$jid );
        $jid ||= defaultJID();
        if ($jid) {
            $jid = resolveJID($jid);
            return unless $jid;
        }
        else {
            owl::error('You must specify an account with -a {jid}');
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
      or die("Usage: jmuc join {muc} [-p password] [-a account]");

    my $presence = new Net::Jabber::Presence;
    $presence->SetPresence( to => $muc );
    my $x = $presence->NewChild('http://jabber.org/protocol/muc');
    $x->AddHistory()->SetMaxChars(0);
    if ($password) {
        $x->SetPassword($password);
    }

    $conn->getConnectionFromJidStr($jid)->Send($presence);
}

sub jmuc_part {
    my ( $jid, $muc, @args ) = @_;

    $muc = shift @args if scalar @args;
    die("Usage: jmuc part {muc} [-a account]") unless $muc;

    $conn->getConnectionFromJidStr($jid)->PresenceSend( to => $muc, type => 'unavailable' );
    queue_admin_msg("$jid has left $muc.");
}

sub jmuc_invite {
    my ( $jid, $muc, @args ) = @_;

    my $invite_jid = shift @args;
    $muc = shift @args if scalar @args;

    die('Usage: jmuc invite {jid} [muc] [-a account]')
      unless $muc && $invite_jid;

    my $message = Net::Jabber::Message->new();
    $message->SetTo($muc);
    my $x = $message->NewChild('http://jabber.org/protocol/muc#user');
    $x->AddInvite();
    $x->GetInvite()->SetTo($invite_jid);
    $conn->getConnectionFromJidStr($jid)->Send($message);
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

    $conn->getConnectionFromJidStr($jid)->Send($iq);
    queue_admin_msg("Accepted default instant configuration for $muc");
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
        owl::error("jroster: Unknown command: $cmd");
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
            $jid = resolveJID($jid);
            return unless $jid;
        }
        else {
            owl::error('You must specify an account with -a {jid}');
        }
        return $func->( $jid, $name, \@groups, $purgeGroups,  @ARGV );
    }
}

sub jroster_sub {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJid = baseJID($jid);

    my $roster = $conn->getRosterFromJidStr($jid);

    # Adding lots of users with the same name is a bad idea.
    $name = "" unless (1 == scalar(@ARGV));

    my $p = new Net::XMPP::Presence;
    $p->SetType('subscribe');

    foreach my $to (@ARGV) {
        jroster_add($jid, $name, \@groups, $purgeGroups, ($to)) unless ($roster->exists($to));

        $p->SetTo($to);
        $conn->getConnectionFromJidStr($jid)->Send($p);
        queue_admin_msg("You ($baseJid) have requested a subscription to ($to)'s presence.");
    }
}

sub jroster_unsub {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJid = baseJID($jid);

    my $p = new Net::XMPP::Presence;
    $p->SetType('unsubscribe');
    foreach my $to (@ARGV) {
        $p->SetTo($to);
        $conn->getConnectionFromJidStr($jid)->Send($p);
        queue_admin_msg("You ($baseJid) have unsubscribed from ($to)'s presence.");
    }
}

sub jroster_add {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJid = baseJID($jid);

    my $roster = $conn->getRosterFromJidStr($jid);

    # Adding lots of users with the same name is a bad idea.
    $name = "" unless (1 == scalar(@ARGV));

    foreach my $to (@ARGV) {
        my %jq  = $roster->query($to);
        my $iq = new Net::XMPP::IQ;
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
        $conn->getConnectionFromJidStr($jid)->Send($iq);
        my $msg = "$baseJid: "
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
    my $baseJid = baseJID($jid);

    my $iq = new Net::XMPP::IQ;
    $iq->SetType('set');
    my $item = new XML::Stream::Node('item');
    $iq->NewChild('jabber:iq:roster')->AddChild($item);
    $item->put_attrib(subscription=> 'remove');
    foreach my $to (@ARGV) {
        $item->put_attrib(jid => $to);
        $conn->getConnectionFromJidStr($jid)->Send($iq);
        queue_admin_msg("You ($baseJid) have removed ($to) from your roster.");
    }
}

sub jroster_auth {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJid = baseJID($jid);

    my $p = new Net::XMPP::Presence;
    $p->SetType('subscribed');
    foreach my $to (@ARGV) {
        $p->SetTo($to);
        $conn->getConnectionFromJidStr($jid)->Send($p);
        queue_admin_msg("($to) has been subscribed to your ($baseJid) presence.");
    }
}

sub jroster_deauth {
    my $jid = shift;
    my $name = shift;
    my @groups = @{ shift() };
    my $purgeGroups = shift;
    my $baseJid = baseJID($jid);

    my $p = new Net::XMPP::Presence;
    $p->SetType('unsubscribed');
    foreach my $to (@ARGV) {
        $p->SetTo($to);
        $conn->getConnectionFromJidStr($jid)->Send($p);
        queue_admin_msg("($to) has been unsubscribed from your ($baseJid) presence.");
    }
}

################################################################################
### Owl Callbacks
sub process_owl_jwrite {
    my $body = shift;

    my $j = new Net::XMPP::Message;
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
    if ( $vars{jwrite}{type} ne 'groupchat' && owl::getvar('displayoutgoing') eq 'on') {
        owl::queue_message($m);
    }

    if ($vars{jwrite}{sid} && $conn->sidExists( $vars{jwrite}{sid} )) {
        $conn->getConnectionFromSid($vars{jwrite}{sid})->Send($j);
    }
    else {
        $conn->getConnectionFromJidStr($vars{jwrite}{from})->Send($j);
    }

    delete $vars{jwrite};
    owl::message("");   # Kludge to make the ``type your message...'' message go away
}

### XMPP Callbacks

sub process_incoming_chat_message {
    my ( $sid, $j ) = @_;
    owl::queue_message( j2o( $j, { direction => 'in',
                                   sid => $sid } ) );
}

sub process_incoming_error_message {
    my ( $sid, $j ) = @_;
    my %jhash = j2hash( $j, { direction => 'in',
                              sid => $sid } );
    $jhash{type} = 'admin';
    owl::queue_message( owl::Message->new(%jhash) );
}

sub process_incoming_groupchat_message {
    my ( $sid, $j ) = @_;

    # HACK IN PROGRESS (ignoring delayed messages)
    return if ( $j->DefinedX('jabber:x:delay') && $j->GetX('jabber:x:delay') );
    owl::queue_message( j2o( $j, { direction => 'in',
                                   sid => $sid } ) );
}

sub process_incoming_headline_message {
    my ( $sid, $j ) = @_;
    owl::queue_message( j2o( $j, { direction => 'in',
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
    owl::queue_message( owl::Message->new(%jhash) );
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
    owl::queue_message(owl::Message->new(%props));
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

    $props{body} = "The user ($from) wants to subscribe to your ($to) presence.\nReply (r) will authorize, reply-sender (R) will deny.";
    $props{replycmd} = "jroster auth $from -a $to";
    $props{replysendercmd} = "jroster deauth $from -a $to";
    owl::queue_message(owl::Message->new(%props));
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
    owl::queue_message(owl::Message->new(%props));

    # Find a connection to reply with.
    foreach my $jid ($conn->getJids()) {
	my $cJid = new Net::XMPP::JID;
	$cJid->SetJID($jid);
	if ($to eq $cJid->GetJID('base') ||
            $to eq $cJid->GetJID('full')) {
	    my $reply = $p->Reply(type=>"unsubscribed");
	    $conn->getConnectionFromJidStr($jid)->Send($reply);
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
    owl::error("Jabber: $code $error");
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
    }
    elsif ( $jtype eq 'groupchat' ) {
        my $nick = $props{nick} = $from->GetResource();
        my $room = $props{room} = $from->GetJID('base');
        $props{replycmd} = "jwrite -g $room";
        $props{replycmd} .=
          " -a " . ( ( $dir eq 'out' ) ? $props{from} : $props{to} );

        $props{sender} = $nick || $room;
        $props{recipient} = $room;

        if ( $props{subject} && !$props{body} ) {
            $props{body} =
              '[' . $nick . " has set the topic to: " . $props{subject} . "]";
        }
    }
    elsif ( $jtype eq 'normal' ) {
        $props{replycmd}  = undef;
        $props{private} = 1;
    }
    elsif ( $jtype eq 'headline' ) {
        $props{replycmd} = undef;
    }
    elsif ( $jtype eq 'error' ) {
        $props{replycmd} = undef;
        $props{body}     = "Error "
          . $props{error_code}
          . " sending to "
          . $props{from} . "\n"
          . $props{error};
    }

    $props{replysendercmd} = $props{replycmd};
    return %props;
}

sub j2o {
    return owl::Message->new( j2hash(@_) );
}

sub queue_admin_msg {
    my $err = shift;
    my $m   = owl::Message->new(
        type      => 'admin',
        direction => 'none',
        body      => $err
    );
    owl::queue_message($m);
}

sub boldify($) {
    my $str = shift;

    return '@b(' . $str . ')' if ( $str !~ /\)/ );
    return '@b<' . $str . '>' if ( $str !~ /\>/ );
    return '@b{' . $str . '}' if ( $str !~ /\}/ );
    return '@b[' . $str . ']' if ( $str !~ /\]/ );

    my $txt = "$str";
    $txt =~ s{[)]}{)\@b[)]\@b(}g;
    return '@b(' . $txt . ')';
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
    return ( $conn->getJids() )[0] if ( $conn->connected() == 1 );
    return;
}

sub baseJID {
    my $givenJidStr = shift;
    my $givenJid    = new Net::XMPP::JID;
    $givenJid->SetJID($givenJidStr);
    return $givenJid->GetJID('base');
}

sub resolveJID {
    my $givenJidStr = shift;
    my $givenJid    = new Net::XMPP::JID;
    $givenJid->SetJID($givenJidStr);

    # Account fully specified.
    if ( $givenJid->GetResource() ) {
        # Specified account exists
        return $givenJidStr if ($conn->jidExists($givenJidStr) );
        owl::error("Invalid account: $givenJidStr");
    }

    # Disambiguate.
    else {
        my $matchingJid = "";
        my $errStr =
          "Ambiguous account reference. Please specify a resource.\n";
        my $ambiguous = 0;

        foreach my $jid ( $conn->getJids() ) {
            my $cJid = new Net::XMPP::JID;
            $cJid->SetJID($jid);
            if ( $givenJidStr eq $cJid->GetJID('base') ) {
                $ambiguous = 1 if ( $matchingJid ne "" );
                $matchingJid = $jid;
                $errStr .= "\t$jid\n";
            }
        }

        # Need further disambiguation.
        if ($ambiguous) {
            queue_admin_msg($errStr);
        }

        # Not one of ours.
        elsif ( $matchingJid eq "" ) {
            owl::error("Invalid account: $givenJidStr");
        }

        # It's this one.
        else {
            return $matchingJid;
        }
    }
    return "";
}

#####################################################################
#####################################################################

package owl::Message::Jabber;

our @ISA = qw( owl::Message );

sub jtype { shift->{jtype} };
sub from { shift->{from} };
sub to { shift->{to} };
sub room { shift->{room} };

sub smartfilter {
    my $self = shift;
    my $inst = shift;

    my ($filter, $ftext);

    if($self->jtype eq 'chat') {
        my $user;
        if($self->direction eq 'in') {
            $user = $self->from;
        } else {
            $user = $self->to;
        }
        $user = Net::Jabber::JID->new($user)->GetJID($inst ? 'full' : 'base');
        $filter = "jabber-user-$user";
        $ftext = qq{type ^jabber\$ and ( ( direction ^in\$ and from ^$user ) } .
                 qq{or ( direction ^out\$ and to ^$user ) ) };
        owl::filter("$filter $ftext");
        return $filter;
    } elsif ($self->jtype eq 'groupchat') {
        my $room = $self->room;
        $filter = "jabber-room-$room";
        $ftext = qq{type ^jabber\$ and room ^$room\$};
        owl::filter("$filter $ftext");
        return $filter;
    }
}

1;
