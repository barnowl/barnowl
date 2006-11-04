package owl_jabber;
use Authen::SASL qw(Perl);
use Net::Jabber;
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

our $client;
our $jid;
our $roster;

sub onStart
{
    if(eval{\&owl::queue_message}) 
    {
	register_owl_commands();
	push @::onMainLoop, sub { owl_jabber::onMainLoop(@_) };
	push @::onGetBuddyList, sub { owl_jabber::onGetBuddyList(@_) };
    }
    else
    {
        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support owl::error. So just
        # give up silently.
    }
}
push @::onStartSubs, sub { owl_jabber::onStart(@_) };

sub onMainLoop
{
    return if ($client == undef);
    
    my $status = $client->Process(0);
    if ($status == 0     # No data received
	|| $status == 1) # Data received
    {
    }
    else #Error
    {
	queue_admin_msg("Jabber disconnected.");
	$roster = undef;
	$client = undef;
	return;
    }
    
    if ($::shutdown)
    {
	$roster = undef;
	$client->Disconnect();
	$client = undef;
	return;
    }
}

sub blist_listBuddy
{
    my $buddy = shift;
    my $blistStr .= "    ";
    my %jq = $roster->query($buddy);
    my $res = $roster->resource($buddy);

    $blistStr .= $jq{name} ? $jq{name} : $buddy->GetJID();
    
    if ($res)
    {
	my %rq = $roster->resourceQuery($buddy, $res);
	$blistStr .= " [".($rq{show} ? $rq{show} : 'online')."]";
	$blistStr .= " ".$rq{status} if $rq{status};
	$blistStr = boldify($blistStr);
    }
    else
    {
	$blistStr .= $jq{ask} ? " [pending]" : " [offline]";
    }

    return $blistStr."\n";
}

sub onGetBuddyList
{
    return "" if ($client == undef);
    my $blist = "\n".boldify("Jabber Roster for ".$jid->GetJID('base'))."\n";

    foreach my $group ($roster->groups())
    {
	$blist .= "  Group: $group\n";
	foreach my $buddy ($roster->jids('group',$group))
	{
	    $blist .= blist_listBuddy($buddy);
	}
    }
    
    my @unsorted = $roster->jids('nogroup');
    if (@unsorted)
    {
	$blist .= "  [unsorted]\n";
	foreach my $buddy (@unsorted)
	{
	    $blist .= blist_listBuddy($buddy);
	}
    }

    $blist .= "\n";
}

################################################################################
### Owl Commands
sub register_owl_commands()
{
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
            summary     => "Send a Jabber Message",
            usage       => "jwrite JID [-g] [-t thread] [-s subject]"
        }
    );
    owl::new_command(
        jlist => \&cmd_jlist,
        {
            summary     => "Show your Jabber roster.",
            usage       => "jlist"
        }
    );
    owl::new_command(
        jmuc => \&cmd_jmuc,
        {
            summary     => "Jabber MUC related commands.",
	    description => "jmuc sends jabber commands related to muc.\n\n".
		"The following commands are available\n\n".
		"join {muc}  Join a muc.\n\n".
		"part [muc]  Part a muc.".
		"            The muc is taken from the current message if not supplied.\n\n".
		"invite {jid} [muc]\n\n".
		"            Invite {jid} to [muc].\n".
		"            The muc is taken from the current message if not supplied.\n\n",
            usage       => "jmuc {command} {args}"
        }
    );
}

sub cmd_login
{
    if ($client != undef)
    {
	queue_admin_msg("Already logged in.");
	return;
    }

    %muc_roster = ();
    $client = Net::Jabber::Client->new();
    $roster = $client->Roster();

    #XXX Todo: Add more callbacks.
    # MUC presence handlers
    $client->SetMessageCallBacks(chat => sub { owl_jabber::process_incoming_chat_message(@_) },
				 error => sub { owl_jabber::process_incoming_error_message(@_) },
				 groupchat => sub { owl_jabber::process_incoming_groupchat_message(@_) },
				 headline => sub { owl_jabber::process_incoming_headline_message(@_) },
				 normal => sub { owl_jabber::process_incoming_normal_message(@_) });

    #XXX Todo: Parameterize the arguments to Connect()
    my $status = $client->Connect(hostname => 'jabber.mit.edu',
				  tls => 1,
				  port => 5222,
				  componentname => 'mit.edu');
    
    if (!$status)
    {
	owl::error("We failed to connect");
	$client = undef;
	return;
    }


    my @result = $client->AuthSend(username => $ENV{USER}, resource => 'owl', password => '');
    if($result[0] ne 'ok') {
	owl::error("Error in connect: " . join(" ", $result[1..$#result]));
	$roster = undef;
	$client->Disconnect();
	$client = undef;
        return;
    }

    $jid = new Net::Jabber::JID;
    $jid->SetJID(userid => $ENV{USER},
		 server => ($client->{SERVER}->{componentname} ||
			    $client->{SERVER}->{hostname}),
		 resource => 'owl');
    
    $roster->fetch();
    $client->PresenceSend(priority => 1);
    queue_admin_msg("Connected to jabber as ".$jid->GetJID('full'));

    return "";
}

sub cmd_logout
{
    if ($client)
    {
	$roster = undef;
	$client->Disconnect();
	$client = undef;
	queue_admin_msg("Jabber disconnected.");
    }
    return "";
}

sub cmd_jlist
{
    if (!$client)
    {
	owl::error("You are not logged in to Jabber.");
	return;
    }
    owl::popless_ztext(onGetBuddyList());
}

our $jwrite_to;
our $jwrite_thread;
our $jwrite_subject;
our $jwrite_type;
sub cmd_jwrite
{
    if (!$client)
    {
	owl::error("You are not logged in to Jabber.");
	return;
    }

    $jwrite_to = "";
    $jwrite_thread = "";
    $jwrite_subject = "";
    $jwrite_type = "chat";
    my @args = @_;
    my $argsLen = @args;

  JW_ARG: for (my $i = 1; $i < $argsLen; $i++)
    {
	$args[$i] =~ /^-t$/ && ($jwrite_thread = $args[++$i]  and next JW_ARG);
	$args[$i] =~ /^-s$/ && ($jwrite_subject = $args[++$i] and next JW_ARG);
	$args[$i] =~ /^-g$/ && ($jwrite_type = "groupchat" and next JW_ARG);

	if ($jwrite_to ne '')
	{
	    # Too many To's
	    $jwrite_to = '';
	    last;
	}
	if ($jwrite_to)
	{
	    $jwrite_to == '';
	    last;
	}
	$jwrite_to = $args[$i];
    }

    if(!$jwrite_to)
    {
        owl::error("Usage: jwrite JID [-t thread] [-s 'subject']");
        return;
    }


    owl::message("Type your message below.  End with a dot on a line by itself.  ^C will quit.");
    owl::start_edit_win(join(' ', @args), \&process_owl_jwrite);
}

sub cmd_jmuc
{
    if (!$client)
    {
	owl::error("You are not logged in to Jabber.");
	return;
    }
    
    if (!$_[1])
    {
	#XXX TODO: Write general usage for jmuc command.
	return;
    }

    my $cmd = $_[1];

    if ($cmd eq 'join')
    {
	if (!$_[2])
	{
	    owl::error('Usage: jmuc join {muc} [password]');
	    return;
	}
	my $muc = $_[2];
	my $x = new XML::Stream::Node('x');
	$x->put_attrib(xmlns => 'http://jabber.org/protocol/muc');
	$x->add_child('history')->put_attrib(maxchars => '0');
	
	if ($_[3]) #password
	{
	    $x->add_child('password')->add_cdata($_[3]);
	}

	my $presence = new Net::Jabber::Presence;
	$presence->SetPresence(to => $muc);
	$presence->AddX($x);
	$client->Send($presence);
    }
    elsif ($cmd eq 'part')
    {
	my $muc;
	if (!$_[2])
	{
	    my $m = owl::getcurmsg();
	    if ($m->is_jabber && $m->{jtype} eq 'groupchat')
	    {
		$muc = $m->{muc};
	    }
	    else
	    {
		owl::error('Usage: "jmuc part [muc]"');
	        return;
	    }
	}
	else
	{
	    $muc = $_[2];
	}
	$client->PresenceSend(to => $muc, type => 'unavailable');
    }
    elsif ($cmd eq 'invite')
    {
	my $jid;
	my $muc;

	owl::error('Usage: jmuc invite {jid} [muc]') if (!$_[2]);
	
	if (!@_[3])
	{  	
	    my $m = owl::getcurmsg();
	    if ($m->is_jabber && $m->{jtype} eq 'groupchat')
	    {
		$muc = $m->{muc};
	    }
	    else
	    {
		owl::error('Usage: jmuc invite {jid} [muc]');
	        return;
	    }
	}
	else
	{
	    $muc = $_[3];
	}
	
	my $x = new XML::Stream::Node('x');
	$x->put_attrib(xmlns => 'http://jabber.org/protocol/muc#user');
	$x->add_child('invite')->put_attrib(to => $_[2]);
	
	my $message = new Net::Jabber::Message;
	$message->SetTo($muc);
	$message->AddX($x);
	
	$client->Send($message);
    }
    else
    {
	owl::error('jmuc: unrecognized command.');
    }
    return "";
}

################################################################################
### Owl Callbacks
sub process_owl_jwrite
{
    my $body = shift;

    my $j = new Net::XMPP::Message;
    $body =~ s/\n\z//;
    $j->SetMessage(to => $jwrite_to,
		   from => $jid->GetJID('full'),
 		   type => $jwrite_type,
 		   body => $body
		   );
    $j->SetThread($jwrite_thread) if ($jwrite_thread);
    $j->SetSubject($jwrite_subject) if ($jwrite_subject);

    my $m = j2o($j, 'out');
    if ($jwrite_type ne 'groupchat')
    {
	#XXX TODO: Check for displayoutgoing.
	owl::queue_message($m);
    }
    $client->Send($j);
}

### XMPP Callbacks

sub process_incoming_chat_message
{
    my ($session, $j) = @_;
    owl::queue_message(j2o($j, 'in'));
}

sub process_incoming_error_message
{
    my ($session, $j) = @_;
    my %jhash = j2hash($j, 'in');
    $jhash{type} = 'admin';
    owl::queue_message(owl::Message->new(%jhash));
}

sub process_incoming_groupchat_message
{
    my ($session, $j) = @_;
    # HACK IN PROGRESS (ignoring delayed messages)
    return if ($j->DefinedX('jabber:x:delay') && $j->GetX('jabber:x:delay'));
    owl::queue_message(j2o($j, 'in'));
}

sub process_incoming_headline_message
{
    my ($session, $j) = @_;
    owl::queue_message(j2o($j, 'in'));
}

sub process_incoming_normal_message
{
    my ($session, $j) = @_;
    my %props = j2hash($j, 'in');

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
    owl::queue_message(owl::Message->new(%props));
}

sub process_muc_presence
{
    my ($session, $p) = @_;
    return unless ($p->HasX('http://jabber.org/protocol/muc#user'));
    
}


### Helper functions

sub j2hash
{
    my $j = shift;
    my $dir = shift;

    my %props = (type => 'jabber',
		 direction => $dir);

    my $jtype = $props{jtype} = $j->GetType();
    my $from  = $j->GetFrom('jid');
    my $to    = $j->GetTo('jid');

    $props{from} = $from->GetJID('full');
    $props{to}   = $to->GetJID('full');

    $props{recipient}  = $to->GetJID('base');
    $props{sender}     = $from->GetJID('base');
    $props{subject}    = $j->GetSubject() if ($j->DefinedSubject());
    $props{thread}     = $j->GetThread() if ($j->DefinedThread());
    $props{body}       = $j->GetBody() if ($j->DefinedBody());
    $props{error}      = $j->GetError() if ($j->DefinedError());
    $props{error_code} = $j->GetErrorCode() if ($j->DefinedErrorCode());
    $props{xml}        = $j->GetXML();

    if ($jtype eq 'chat')
    {
	$props{replycmd} = "jwrite ".(($dir eq 'in') ? $props{from} : $props{to});
	$props{isprivate} = 1;
    }
    elsif ($jtype eq 'groupchat')
    {
	my $nick = $props{nick} = $from->GetResource();
	my $room = $props{room} = $from->GetJID('base');
	$props{replycmd} = "jwrite -g $room";
	
	$props{sender} = $nick;
	$props{recipient} = $room;

	if ($props{subject} && !$props{body})
	{
	    $props{body} = '['.$nick." has set the topic to: ".$props{subject}."]"
	}
    }
    elsif ($jtype eq 'normal')
    {
	$props{replycmd} = undef;
	$props{isprivate} = 1;
    }
    elsif ($jtype eq 'headline')
    {
	$props{replycmd} = undef;
    }
    elsif ($jtype eq 'error')
    {
	$props{replycmd} = undef;
	$props{body} = "Error ".$props{error_code}." sending to ".$props{from}."\n".$props{error};
    }
    
    $props{replysendercmd} = $props{replycmd};
    return %props;
}

sub j2o
{
    return owl::Message->new(j2hash(@_));
}

sub queue_admin_msg
{
    my $err = shift;
    my $m = owl::Message->new(type => 'admin',
			      direction => 'none',
			      body => $err);
    owl::queue_message($m);
}

sub boldify($)
{
    $str = shift;

    return '@b('.$str.')' if ( $str !~ /\)/ );
    return '@b<'.$str.'>' if ( $str !~ /\>/ );
    return '@b{'.$str.'}' if ( $str !~ /\}/ );
    return '@b['.$str.']' if ( $str !~ /\]/ );

    my $txt = "\@b($str";
    $txt =~ s/\)/\)\@b\[\)\]\@b\(/g;
    return $txt.')';
}

