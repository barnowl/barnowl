package owl_jabber;
use Authen::SASL qw(Perl);
use Net::Jabber;
################################################################################
# owl perl jabber support
#
# Todo:
# Connect command.
#
################################################################################

our $client;
our $jid;

sub onStart
{
    if(eval{\&owl::queue_message}) 
    {
	register_owl_commands();
    }
    else
    {
        # Our owl doesn't support queue_message. Unfortunately, this
        # means it probably *also* doesn't support owl::error. So just
        # give up silently.
    }
}
push @::onStartSubs, \&onStart;

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
	$client = undef;
	return;
    }
    
    if ($::shutdown)
    {
	$client->Disconnect();
	$client = undef;
	return;
    }
}
push @::onMainLoop, \&onMainLoop;

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
            usage       => "jwrite JID [-t thread]"
        }
    );
    owl::new_command(
        jchat => \&cmd_jwrite_gc,
        {
            summary => "Send a Jabber Message",
            usage       => "jchat [room]@[server]"
        }
    );
    owl::new_command(
        jjoin => \&cmd_join_gc,
        {
            summary     => "Joins a jabber groupchat.",
            usage       => "jjoin [room]@[server]/[nick]"
        }
    );
    owl::new_command(
        jpart => \&cmd_part_gc,
        {
            summary     => "Parts a jabber groupchat.",
            usage       => "jpart [room]@[server]/[nick]"
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
    
    # These strings should not be hard-coded here.
    $client = Net::Jabber::Client->new();
    $client->SetMessageCallBacks(chat => sub { owl_jabber::process_incoming_chat_message(@_) },
				 error => sub { owl_jabber::process_incoming_error_message(@_) },
				 groupchat => sub { owl_jabber::process_incoming_groupchat_message(@_) },
				 headline => sub { owl_jabber::process_incoming_headline_message(@_) },
				 normal => sub { owl_jabber::process_incoming_normal_message(@_) });
    my $status = $client->Connect(hostname => 'jabber.mit.edu',
				  tls => 1,
				  port => 5222,
				  componentname => 'mit.edu');
    
    if (!$status)
    {
	owl::error("We failed to connect");
	return;
    }

    my @result = $client->AuthSend(username => $ENV{USER}, resource => 'owl', password => '');
    if($result[0] ne 'ok') {
	owl::error("Error in connect: " . join(" ", $result[1..$#result]));
	$client->Disconnect();
	$client = undef;
        return;
    }

    $jid = new Net::Jabber::JID;
    $jid->SetJID(userid => $ENV{USER},
		 server => ($client->{SERVER}->{componentname} ||
			    $client->{SERVER}->{hostname}),
		 resource => 'owl');
    
    $client->PresenceSend(priority => 1);
    queue_admin_msg("Connected to jabber as ".$jid->GetJID('full'));

    return "";
}

sub cmd_logout
{
    if ($client)
    {
	$client->Disconnect();
	$client = undef;
	queue_admin_msg("Jabber disconnected.");
    }
    return "";
}

our $jwrite_to;
our $jwrite_thread;
our $jwrite_subject;
our $jwrite_type;
sub cmd_jwrite
{
    if (!$client)
    {
	# Error here
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
	$args[$i] =~ /^-t$/ && ($jwrite_thread = $args[++$i]  && next JW_ARG);
	$args[$i] =~ /^-s$/ && ($jwrite_subject = $args[++$i] && next JW_ARG);
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

    if(!$jwrite_to) {
        owl::error("Usage: jwrite JID [-t thread] [-s 'subject']");
        return;
    }
    
    owl::message("Type your message below.  End with a dot on a line by itself.  ^C will quit.");
    owl::start_edit_win(join(' ', @args), \&process_owl_jwrite);
}

sub cmd_join_gc
{
    if (!$client)
    {
	# Error here
	return;
    }
    if(!$_[1])
    {
	owl::error("Usage: jchat [room]@[server]/[nick]");
	return;
    }

    my $x = new XML::Stream::Node('x');
    $x->put_attrib(xmlns => 'http://jabber.org/protocol/muc');
    $x->add_child('history')->put_attrib(maxchars => '0');


    my $presence = new Net::Jabber::Presence;
    $presence->SetPresence(to => $_[1]);
    $presence->AddX($x);

    $client->Send($presence);
    return "";
}

sub cmd_part_gc
{
    if (!$client)
    {
	# Error here
	return;
    }
    if(!$_[1])
    {
	owl::error("Usage: jchat [room]@[server]/[nick]");
	return;
    }

    $client->PresenceSend(to=>$_[1], type=>'unavailable');
    return "";
}

sub cmd_jwrite_gc
{
    if (!$client)
    {
	# Error here
	return;
    }

    $jwrite_to = $_[1];
    $jwrite_thread = "";
    $jwrite_subject = "";
    $jwrite_type = "groupchat";
    my @args = @_;
    my $argsLen = @args;

    owl::message("Type your message below.  End with a dot on a line by itself.  ^C will quit.");
    owl::start_edit_win(join(' ', @args), \&process_owl_jwrite);
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
    queue_admin_msg("Error ".$j->GetErrorCode()." sending to ".$j->GetFrom('jid')->GetJID('base'));
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
    owl::queue_message(j2o($j, 'in'));
}


### Helper functions

sub j2o
{
    my $j = shift;
    my $dir = shift;

    my %props = (type => 'jabber',
		 direction => $dir);


    $props{replycmd} = "jwrite";

    $props{jtype} = $j->GetType();
    $props{jtype} =~ /^(?:headline|error)$/ && {$props{replycmd} = undef};
    $props{jtype} =~ /^groupchat$/ && {$props{replycmd} = "jchat"};

    $props{isprivate} = $props{jtype} =~ /^(?:normal|chat)$/;

    my $reply_to;
    if ($j->DefinedTo())
    {
	my $jid = $j->GetTo('jid');
	$props{recipient} = $jid->GetJID('base');
	$props{to_jid} = $jid->GetJID('full');
	if ($dir eq 'out')
	{
	    $reply_to = $props{to_jid};
	    $props{jtype} =~ /^groupchat$/ && {$reply_to = $jid->GetJID('base')};
	}
    }
    if ($j->DefinedFrom())
    {
	my $jid = $j->GetFrom('jid');
	$props{sender} = $jid->GetJID('base');
	$props{from_jid} = $jid->GetJID('full');
	$reply_to = $props{from_jid} if ($dir eq 'in');
	if ($dir eq 'in')
	{
	    $reply_to = $props{from_jid};
	    $props{jtype} =~ /^groupchat$/ && {$reply_to = $jid->GetJID('base')};
	}
    }

    $props{subject} = $j->GetSubject() if ($j->DefinedSubject());
    $props{body} = $j->GetBody() if ($j->DefinedBody());
#    if ($j->DefinedThread())
#    {
#	$props{thread} = $j->GetThread() if ($j->DefinedThread());
#	$props{replycmd} .= " -t $props{thread}";
#    }
    $props{error} = $j->GetError() if ($j->DefinedError());
    $props{error_code} = $j->GetErrorCode() if ($j->DefinedErrorCode());
    $props{replycmd} .= " $reply_to";
    $props{replysendercmd} = $props{replycmd};

    return owl::Message->new(%props);
}

sub queue_admin_msg
{
    my $err = shift;
    my $m = owl::Message->new(type => 'admin',
			      direction => 'none',
			      body => $err);
    owl::queue_message($m);
}
