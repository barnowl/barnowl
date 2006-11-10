# -*- mode: cperl; cperl-indent-level: 4; indent-tabs-mode: nil -*-
package owl_jabber;
use warnings;
use strict;

use Authen::SASL qw(Perl);
use Net::Jabber;
use Net::DNS;
use Getopt::Long;

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
#  * Current behavior => auto-accept (default for Net::Jabber)
#
################################################################################

our $connections;
our %vars;

sub onStart {
    if ( eval { \&owl::queue_message } ) {
        register_owl_commands();
        push @::onMainLoop,     sub { owl_jabber::onMainLoop(@_) };
        push @::onGetBuddyList, sub { owl_jabber::onGetBuddyList(@_) };
    }
    else {

        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support owl::error. So just
        # give up silently.
    }
}

push @::onStartSubs, sub { owl_jabber::onStart(@_) };

sub onMainLoop {
    return if ( !connected() );

    foreach my $jid ( keys %$connections ) {
        my $client = \$connections->{$jid}->{client};

        my $status = $$client->Process(0);
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
    my %jq  = $$roster->query($buddy);
    my $res = $$roster->resource($buddy);

    $blistStr .= $jq{name} ? $jq{name} : $buddy->GetJID();

    if ($res) {
        my %rq = $$roster->resourceQuery( $buddy, $res );
        $blistStr .= " [" . ( $rq{show} ? $rq{show} : 'online' ) . "]";
        $blistStr .= " " . $rq{status} if $rq{status};
        $blistStr = boldify($blistStr);
    }
    else {
        $blistStr .= $jq{ask} ? " [pending]" : " [offline]";
    }

    return $blistStr . "\n";
}

sub onGetBuddyList {
    my $blist = "";
    foreach my $jid ( keys %{$connections} ) {
        my $roster = \$connections->{$jid}->{roster};
        if ($$roster) {
            $blist .= "\n" . boldify("Jabber Roster for $jid\n");

            foreach my $group ( $$roster->groups() ) {
                $blist .= "  Group: $group\n";
                foreach my $buddy ( $$roster->jids( 'group', $group ) ) {
                    $blist .= blist_listBuddy( $roster, $buddy );
                }
            }

            my @unsorted = $$roster->jids('nogroup');
            if (@unsorted) {
                $blist .= "  [unsorted]\n";
                foreach my $buddy (@unsorted) {
                    $blist .= blist_listBuddy( $roster, $buddy );
                }
            }
        }
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
              . "configure [muc]\n" "            Configure [muc].\n"
              . "            Necessary to initalize a new MUC",
            usage => "jmuc {command} {args}"
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

    if ( $connections->{$jidStr} ) {
        owl::error("Already logged in as $jidStr.");
        return;
    }

    my ( $server, $port ) = getServerFromJID($jid);

    $connections->{$jidStr}->{client} = Net::Jabber::Client->new(
        debuglevel => owl::getvar('debug') eq 'on' ? 1 : 0,
        debugfile => 'jabber.log'
    );
    my $client = \$connections->{$jidStr}->{client};
    $connections->{$jidStr}->{roster} =
      $connections->{$jidStr}->{client}->Roster();

    #XXX Todo: Add more callbacks.
    # MUC presence handlers
    $$client->SetMessageCallBacks(
        chat      => sub { owl_jabber::process_incoming_chat_message(@_) },
        error     => sub { owl_jabber::process_incoming_error_message(@_) },
        groupchat => sub { owl_jabber::process_incoming_groupchat_message(@_) },
        headline  => sub { owl_jabber::process_incoming_headline_message(@_) },
        normal    => sub { owl_jabber::process_incoming_normal_message(@_) }
    );

    $vars{jlogin_connhash} = {
        hostname      => $server,
        tls           => 1,
        port          => $port,
        componentname => $componentname
    };

    my $status = $$client->Connect( %{ $vars{jlogin_connhash} } );

    if ( !$status ) {
        delete $connections->{$jidStr};
        delete $vars{jlogin_connhash};
        owl::error("We failed to connect");
        return "";
    }

    $vars{jlogin_authhash} =
      { username => $uid, resource => $resource, password => '' };
    my @result = $$client->AuthSend( %{ $vars{jlogin_authhash} } );
    if ( $result[0] ne 'ok' ) {
        if ( $result[1] == 401 ) {
            $vars{jlogin_jid} = $jidStr;
            delete $connections->{$jidStr};
            owl::start_password( "Password for $jidStr: ", \&do_login_with_pw );
            return "";
        }
        owl::error(
            "Error in connect: " . join( " ", $result[ 1 .. $#result ] ) );
        do_logout($jidStr);
        delete $vars{jlogin_connhash};
        delete $vars{jlogin_authhash};
        return "";
    }
    $connections->{$jidStr}->{roster}->fetch();
    $$client->PresenceSend( priority => 1 );
    queue_admin_msg("Connected to jabber as $jidStr");
    delete $vars{jlogin_connhash};
    delete $vars{jlogin_authhash};
    return "";
}

sub do_login_with_pw {
    $vars{jlogin_authhash}->{password} = shift;
    my $jidStr = delete $vars{jlogin_jid};
    if ( !$jidStr ) {
        owl::error("Got password but have no jid!");
    }

    $connections->{$jidStr}->{client} = Net::Jabber::Client->new();
    my $client = \$connections->{$jidStr}->{client};
    $connections->{$jidStr}->{roster} =
      $connections->{$jidStr}->{client}->Roster();

    $$client->SetMessageCallBacks(
        chat      => sub { owl_jabber::process_incoming_chat_message(@_) },
        error     => sub { owl_jabber::process_incoming_error_message(@_) },
        groupchat => sub { owl_jabber::process_incoming_groupchat_message(@_) },
        headline  => sub { owl_jabber::process_incoming_headline_message(@_) },
        normal    => sub { owl_jabber::process_incoming_normal_message(@_) }
    );

    my $status = $$client->Connect( %{ $vars{jlogin_connhash} } );
    if ( !$status ) {
        delete $connections->{$jidStr};
        delete $vars{jlogin_connhash};
        delete $vars{jlogin_authhash};
        owl::error("We failed to connect");
        return "";
    }

    my @result = $$client->AuthSend( %{ $vars{jlogin_authhash} } );

    if ( $result[0] ne 'ok' ) {
        owl::error(
            "Error in connect: " . join( " ", $result[ 1 .. $#result ] ) );
        do_logout($jidStr);
        delete $vars{jlogin_connhash};
        delete $vars{jlogin_authhash};
        return "";
    }

    $connections->{$jidStr}->{roster}->fetch();
    $$client->PresenceSend( priority => 1 );
    queue_admin_msg("Connected to jabber as $jidStr");
    delete $vars{jlogin_connhash};
    delete $vars{jlogin_authhash};
    return "";
}

sub do_logout {
    my $jid = shift;
    $connections->{$jid}->{client}->Disconnect();
    delete $connections->{$jid};
    queue_admin_msg("Jabber disconnected ($jid).");
}

sub cmd_logout {

    # Logged into multiple accounts
    if ( connected() > 1 ) {

        # Logged into multiple accounts, no accout specified.
        if ( !$_[1] ) {
            my $errStr =
"You are logged into multiple accounts. Please specify an account to log out of.\n";
            foreach my $jid ( keys %$connections ) {
                $errStr .= "\t$jid\n";
            }
            queue_admin_msg($errStr);
        }

        # Logged into multiple accounts, account specified.
        else {
            if ( $_[1] eq '-a' )    #All accounts.
            {
                foreach my $jid ( keys %$connections ) {
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

        do_logout( ( keys %$connections )[0] );
    }
    return "";
}

sub cmd_jlist {
    if ( !( scalar keys %$connections ) ) {
        owl::error("You are not logged in to Jabber.");
        return;
    }
    owl::popless_ztext( onGetBuddyList() );
}

sub cmd_jwrite {
    if ( !connected() ) {
        owl::error("You are not logged in to Jabber.");
        return;
    }

    my $jwrite_to      = "";
    my $jwrite_from    = "";
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
        if ( connected() == 1 ) {
            $jwrite_from = ( keys %$connections )[0];
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
    die "You are not logged in to Jabber" unless connected();
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

    my $x = new XML::Stream::Node('x');
    $x->put_attrib( xmlns => 'http://jabber.org/protocol/muc' );
    $x->add_child('history')->put_attrib( maxchars => '0' );

    if ($password) {
        $x->add_child('password')->add_cdata($password);
    }

    my $presence = new Net::Jabber::Presence;
    $presence->SetPresence( to => $muc );
    $presence->AddX($x);
    $connections->{$jid}->{client}->Send($presence);
}

sub jmuc_part {
    my ( $jid, $muc, @args ) = @_;

    $muc = shift @args if scalar @args;
    die("Usage: jmuc part {muc} [-a account]") unless $muc;

    $connections->{$jid}->{client}
      ->PresenceSend( to => $muc, type => 'unavailable' );
    queue_admin_msg("$jid has left $muc.");
}

sub jmuc_invite {
    my ( $jid, $muc, @args ) = @_;

    my $invite_jid = shift @args;
    $muc = shift @args if scalar @args;

    die('Usage: jmuc invite {jid} [muc] [-a account]')
      unless $muc && $invite_jid;

    my $x = new XML::Stream::Node('x');
    $x->put_attrib( xmlns => 'http://jabber.org/protocol/muc#user' );
    $x->add_child('invite')->put_attrib( to => $invite_jid );

    my $message = new Net::Jabber::Message;
    $message->SetTo($muc);
    $message->AddX($x);
    $connections->{$jid}->{client}->Send($message);
    queue_admin_msg("$jid has invited $invite_jid to $muc.");
}

Net::Jabber::Namespaces::add_ns(
    ns  => "http://jabber.org/protocol/muc#owner",
    tag => 'query',
);

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

    $connections->{$jid}->{client}->Send($iq);
    queue_admin_msg("Accepted default instant configuration for $muc");
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

    my $m = j2o( $j, 'out' );
    if ( $vars{jwrite}{type} ne 'groupchat' ) {

        #XXX TODO: Check for displayoutgoing.
        owl::queue_message($m);
    }
    $connections->{ $vars{jwrite}{from} }->{client}->Send($j);
    delete $vars{jwrite};
}

### XMPP Callbacks

sub process_incoming_chat_message {
    my ( $session, $j ) = @_;
    owl::queue_message( j2o( $j, 'in' ) );
}

sub process_incoming_error_message {
    my ( $session, $j ) = @_;
    my %jhash = j2hash( $j, 'in' );
    $jhash{type} = 'admin';
    owl::queue_message( owl::Message->new(%jhash) );
}

sub process_incoming_groupchat_message {
    my ( $session, $j ) = @_;

    # HACK IN PROGRESS (ignoring delayed messages)
    return if ( $j->DefinedX('jabber:x:delay') && $j->GetX('jabber:x:delay') );
    owl::queue_message( j2o( $j, 'in' ) );
}

sub process_incoming_headline_message {
    my ( $session, $j ) = @_;
    owl::queue_message( j2o( $j, 'in' ) );
}

sub process_incoming_normal_message {
    my ( $session, $j ) = @_;
    my %props = j2hash( $j, 'in' );

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
    owl::queue_message( owl::Message->new(%props) );
}

sub process_muc_presence {
    my ( $session, $p ) = @_;
    return unless ( $p->HasX('http://jabber.org/protocol/muc#user') );

}

### Helper functions

sub j2hash {
    my $j   = shift;
    my $dir = shift;

    my %props = (
        type      => 'jabber',
        direction => $dir
    );

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
        $props{isprivate} = 1;
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
        $props{isprivate} = 1;
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

    my $txt = "\@b($str";
    $txt =~ s/\)/\)\@b\[\)\]\@b\(/g;
    return $txt . ')';
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

sub connected {
    return scalar keys %$connections;
}

sub defaultJID {
    return ( keys %$connections )[0] if ( connected() == 1 );
    return;
}

sub resolveJID {
    my $givenJidStr = shift;
    my $givenJid    = new Net::XMPP::JID;
    $givenJid->SetJID($givenJidStr);

    # Account fully specified.
    if ( $givenJid->GetResource() ) {

        # Specified account exists
        if ( defined $connections->{$givenJidStr} ) {
            return $givenJidStr;
        }
        else    #Specified account doesn't exist
        {
            owl::error("Invalid account: $givenJidStr");
        }
    }

    # Disambiguate.
    else {
        my $matchingJid = "";
        my $errStr =
          "Ambiguous account reference. Please specify a resource.\n";
        my $ambiguous = 0;

        foreach my $jid ( keys %$connections ) {
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

        # Log out this one.
        else {
            return $matchingJid;
        }
    }
    return "";
}
