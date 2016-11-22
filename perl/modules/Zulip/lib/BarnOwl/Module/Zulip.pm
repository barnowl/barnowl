use strict;
use warnings;

package BarnOwl::Module::Zulip;

our $VERSION=0.1;
our $queue_id;
our $last_event_id;
our %cfg;
our $max_retries = 1000;
our $retry_timer;
our $tls_ctx;
our %msg_id_map;
our $presence_timer;

use AnyEvent;
use AnyEvent::HTTP;
use AnyEvent::TLS;
use JSON;
use MIME::Base64;
use URI;
use URI::Encode;
use BarnOwl::Hooks;
use BarnOwl::Message::Zulip;
use HTTP::Request::Common;
use Getopt::Long qw(GetOptionsFromArray);

sub fail {
    my $msg = shift;
    undef %cfg;
    undef $queue_id;
    undef $last_event_id;

    BarnOwl::admin_message('Zulip Error', $msg);
    die("Zulip Error: $msg\n");
}


sub initialize {
    my $conffile = BarnOwl::get_config_dir() . "/zulip";

    if (open(my $fh, "<", "$conffile")) {
        read_config($fh);
        close($fh);
    }
    if ($cfg{'api_url'} =~ /^https/) {
        if (exists $cfg{'ssl_key_file'}) {
            $tls_ctx = new AnyEvent::TLS(verify => $cfg{'ssl_verify'}, 
                                         sslv3 => 0, verify_peername => "http",
                                         ca_file => $cfg{'ssl_ca_file'},
                                         cert_file => $cfg{'ssl_cert_file'},
                                         key_file => $cfg{'ssl_key_file'});
        } else {
            $tls_ctx = new AnyEvent::TLS(verify => $cfg{'ssl_verify'}, 
                                         sslv3 => 0, verify_peername => "http",
                                         ca_file => $cfg{'ssl_ca_file'});
        }           
    } else {
        # we still want it for a unique id
        $tls_ctx = int(rand(1024));
    }
}


sub authorization {
    return "Basic " . encode_base64($cfg{'user'} . ':' . $cfg{'apikey'}, "")
}

sub read_config {
    my $fh = shift;

    my $raw_cfg = do {local $/; <$fh>};
    close($fh);
    eval {
        $raw_cfg = from_json($raw_cfg);
    };
    if($@) {
        fail("Unable to parse config file: $@");
    }

    if(! exists $raw_cfg->{user}) {
        fail("Account has no username set.");
    }
    my $user = $raw_cfg->{user};
    if(! exists $raw_cfg->{apikey}) {
        fail("Account has no api key set.");
    }
    my $apikey = $raw_cfg->{apikey};
    if(! exists $raw_cfg->{api_url}) {
        fail("Account has no API url set.");
    }
    my $api_url = $raw_cfg->{api_url};

    if(! exists $raw_cfg->{default_realm}) {
        fail("Account has no default realm set.");
    }
    my $default_realm = $raw_cfg->{default_realm};

    if( exists $raw_cfg->{ssl}) {
        # mandatory parameters
        if (! exists $raw_cfg->{ssl}->{ca_file}) {
            fail("SSL parameters specified, but no CA file set");
        }
        $cfg{'ssl_ca_file'} = $raw_cfg->{ssl}->{ca_file};
        $cfg{'ssl_verify'} = 1;
        # optional parameters
        if ( (exists $raw_cfg->{ssl}->{cert_file}) && exists $raw_cfg->{ssl}->{key_file}) {
            $cfg{'ssl_cert_file'} = $raw_cfg->{ssl}->{cert_file};
            $cfg{'ssl_key_file'} = $raw_cfg->{ssl}->{key_file};
        }  else {
            warn "SSL parameters specified, but no client credentials set.";
        }
    } else {
	$cfg{'ssl_verify'} = 0;
	my $msg = "SSL parameters not specified. WILL NOT VERIFY SERVER CERTIFICATE. See README for details.";
	BarnOwl::admin_message('Zulip Warning', "Zulip: $msg");
	warn $msg;
    }
    
    $cfg{'user'} = $user;
    $cfg{'apikey'} = $apikey;
    $cfg{'api_url'} = $api_url;
    $cfg{'realm'} = $default_realm;
}

sub register {
    my $retry_count = 0;
    my $callback;

    if(!exists $cfg{'api_url'}) {
        die("Zulip not configured, cannot poll for events");
    }
    $callback = sub {
        BarnOwl::debug("In register callback");
        my ($body, $headers) = @_;
        if($headers->{Status} > 399) {
            warn "Failed to make event queue registration request ($headers->{Status})";
            if($retry_count >= $max_retries) {
                fail("Giving up");
            } else {
                $retry_count++;                       
                http_post($cfg{'api_url'} . "/register", "",
                          session => $tls_ctx,
                          sessionid => $tls_ctx,
                          tls_ctx => $tls_ctx,
                          headers => { "Authorization" => authorization }, 
                          $callback);
                return;
            }
        } else {
            my $response = decode_json($body);
            if($response->{result} ne "success") {
                fail("Failed to register event queue; error was $response->{msg}");
            } else {
                $last_event_id = $response->{last_event_id};
                $queue_id = $response->{queue_id};
                $presence_timer = AnyEvent->timer(after => 1, interval => 60, cb => sub {
                    my $presence_url = $cfg{'api_url'} . "/users/me/presence";
                    my %presence_params = (status => "active", new_user_input => "true");
                    my $presence_body = POST($presence_url, \%presence_params)->content;
                    http_post($presence_url, $presence_body, headers => { "Authorization" => authorization,
                                                                          "Content-Type" => "application/x-www-form-urlencoded" },
                              session => $tls_ctx,
                              sessionid => $tls_ctx,
                              tls_ctx => $tls_ctx,
                              sub {
                                  my ($body, $headers) = @_;
                                  if($headers->{Status} > 399) {
                                      warn("Error sending presence");
                                      warn(encode_json($headers));
                                      warn($body);
                                  }});
                    return;});
                do_poll();
                return;
            }
        }
    };
    
    http_post($cfg{'api_url'} . "/register", "",
              headers => { "Authorization" => authorization }, 
              session => $tls_ctx,
              sessionid => $tls_ctx,
              tls_ctx => $tls_ctx,
              $callback);
    return;
}

sub parse_response {
    my ($body, $headers) = @_;
    if($headers->{Status} > 399) {
        return 0;
    }
    my $response = decode_json($body);
    if($response->{result} ne "success") {
        return 0;
    }
    return $response;
}
sub do_poll {
    my $uri = URI->new($cfg{'api_url'} . "/events");
    $uri->query_form("queue_id" => $queue_id, 
                     "last_event_id" => $last_event_id);
    my $retry_count = 0;
    my $callback;
    $callback = sub {
        my ($body, $headers) = @_;
        my $response = parse_response($body, $headers);
        if(!$response) {
            warn "Failed to poll for events in do_poll: $headers->{Reason}";
            if($retry_count >= $max_retries) {
                warn "Retry count exceeded in do_poll, giving up";
                fail("do_poll: Giving up");
                $presence_timer->cancel;
            } else {
                warn "Retrying";
                $retry_count++;       
                $retry_timer = AnyEvent->timer(after => 10, cb => sub { warn "retry number $retry_count"; 
                                                         http_get($uri->as_string, 
                                                                  "headers" => { "Authorization" => authorization },
                                                                  session => $tls_ctx,
                                                                  sessionid => $tls_ctx,
                                                                  tls_ctx => $tls_ctx, 
                                                                  $callback);
                                                         return;
                                });
                return;
            }
        } else {
            event_cb($response);
        }
    };
    http_get($uri->as_string, "headers" => { "Authorization" => authorization }, 
             session => $tls_ctx,
             sessionid => $tls_ctx,
             tls_ctx => $tls_ctx,$callback);
    return;
}

sub event_cb {
    my $response = $_[0];
    if($response->{result} ne "success") {
        fail("event_cb: Failed to poll for events; error was $response->{msg}");
    } else {
        for my $event (@{$response->{events}}) {
            if($event->{type} eq "message") {
                my $msg = $event->{message};
                my %msghash = (
                    type => 'Zulip',
                    sender => $msg->{sender_email},
                    recipient => $msg->{recipient_id},
                    direction => 'in',
                    class => $msg->{display_recipient},
                    instance => $msg->{subject},
                    unix_time => $msg->{timestamp},
                    source => "zulip",
                    location => "zulip",
                    body => $msg->{content},
                    zid => $msg->{id},
                    sender_full_name => $msg->{sender_full_name},
                    opcode => "");
                $msghash{'body'} =~ s/\r//gm;
                if($msg->{type} eq "private") {
                    $msghash{private} = 1;
                    my @raw_recipients = @{$msg->{display_recipient}};
                    my @display_recipients;
                    if (scalar(@raw_recipients) > 1) {
                        my $recip;
                        for $recip (@raw_recipients) {
                            unless ($recip->{email} eq $cfg{user}) {
                                push @display_recipients, $recip->{email};
                            }
                        }
                        $msghash{recipient} = join " ", @display_recipients;
                    } else {
                        $msghash{recipient} = $msg->{display_recipient}->[0]->{email};
                    }
                    $msghash{class} = "message";
                    if($msg->{sender_email} eq $cfg{user}) {
                        $msghash{direction} = 'out';
                    }
                }
                my $bomsg = BarnOwl::Message->new(%msghash);
                # queue_message returns the message round-tripped
                # through owl_message. In particular, this means it
                # has a meaningful id.
                my $rtmsg = BarnOwl::queue_message($bomsg);
                # note that only base messages, not edits, end up in
                # here. Tim promises me that we will never see an
                # update to an update, so we shouldn't need to
                # retrieve updated messages via this
                $msg_id_map{$rtmsg->zid} = $rtmsg->id;
            } elsif($event->{type} eq "update_message") {
                my $id = $event->{message_id};
                if(!exists $msg_id_map{$id}) {
                    BarnOwl::debug("Got update for unknown message $id, discarding");
                } else {
                    my $base_msg = BarnOwl::get_message_by_id($msg_id_map{$id});
                    my %new_msghash = (
                        type => 'Zulip',
                        sender => $base_msg->sender,
                        recipient => $base_msg->recipient,
                        direction => $base_msg->direction,
                        class => $base_msg->class,
                        # instance needs to be potentially determined from new message
                        unix_time => $event->{edit_timestamp},
                        source => "zulip",
                        location => "zulip",
                        # content needs to be potentially determined from new message
                        zid => $base_msg->id,
                        sender_full_name => $base_msg->long_sender,
                        opcode => "EDIT");
                    if (exists $$event{'subject'}) {
                        $new_msghash{'instance'} = $event->{subject};
                    } else {
                        $new_msghash{'instance'} = $base_msg->instance;
                    }
                    if (exists $$event{'content'}) {
                        $new_msghash{'body'} = $event->{content};
                    } else {
                        $new_msghash{'body'} = $base_msg->body;
                    }
                    my $bomsg = BarnOwl::Message->new(%new_msghash);
                    BarnOwl::queue_message($bomsg);
                }
                
            } else {
                BarnOwl::debug("Got unknown message");
                BarnOwl::debug(encode_json($event));
            }
            $last_event_id = $event->{id};
            do_poll();
            return;
        }
    }
    
}

sub zulip {
    my ($type, $recipient, $subject, $msg) = @_;
    # only care about it for its url encoding
    my $builder = URI->new("http://www.example.com");
    my %params = ("type" => $type, "to" => $recipient,  "content" => $msg);
    if ($subject ne "") {
        $params{"subject"} = $subject;
    }
    my $url = $cfg{'api_url'} . "/messages";
    my $req = POST($url, \%params); 
    http_post($url, $req->content, "headers" => {"Authorization" => authorization, "Content-Type" => "application/x-www-form-urlencoded"}, 
              session => $tls_ctx,
              sessionid => $tls_ctx,
              tls_ctx => $tls_ctx,sub { 
                  my ($body, $headers) = @_;
                  if($headers->{Status} < 400) {
                      BarnOwl::message("Zulipgram sent");
                  } else {
                      BarnOwl::message("Error sending zulipgram: $headers->{Reason}!");
                      BarnOwl::debug($body);
                  }});
    return;
}


sub update_subs {
    my ($add_list, $remove_list, $cb) = @_;
    my @add_param = ();
    my @remove_param = ();
    for my $add (@$add_list) {
        push @add_param, {name => $add};
    }
    my $url = $cfg{'api_url'} . "/users/me/subscriptions";
    my %params = ("add" => encode_json(\@add_param), "delete" => encode_json($remove_list));
    my $req = POST($url, \%params);
    http_request('PATCH' => $url,
                 "body" => $req->content,
                 "headers" => {"Authorization" => authorization, "Content-Type" => "application/x-www-form-urlencoded"},
              session => $tls_ctx,
              sessionid => $tls_ctx,
              tls_ctx => $tls_ctx,sub { 
                  my ($body, $headers) = @_;
                  if($headers->{Status} < 400) {
                      &$cb();
                  } else {
                      BarnOwl::message("Error updating subscriptions: $headers->{Reason}!");
                      BarnOwl::debug($body);
                  }});
    return;
}

sub get_subs {
    my $url = $cfg{'api_url'} . "/users/me/subscriptions";
    http_get($url, headers => { "Authorization" => authorization },
             session => $tls_ctx, sessionid => $tls_ctx,
             tls_ctx => $tls_ctx, sub {
                 my ($body, $headers) = @_;
                 if ($headers->{Status} > 399) {
                     BarnOwl::message("Error retrieving subscription list: $headers->{Reason}");
                     BarnOwl::debug($body);
                 }
                 my $data = decode_json($body);
                 my @subs;
                 for my $s (@{$data->{subscriptions}}) {
                     push @subs, $s->{name};
                 }
                 BarnOwl::popless_text(join "\n", @subs);
             });
    return;
}

sub cmd_zulip_sub {
    my ($cmd, $stream) = @_;
    update_subs([$stream], [], sub {
        BarnOwl::message("Subscribed to $stream");});
    
}

sub cmd_zulip_unsub {
    my ($cmd, $stream) = @_;
    update_subs([], [$stream], sub {
        BarnOwl::message("Unsubscribed from $stream");});
    
}

sub cmd_zulip_getsubs {
    get_subs();
}

sub cmd_zulip_login {
    register();
}

sub cmd_zulip_write {
    my $cmdline = join " ", @_;
    my $cmd = shift;
    my $stream;
    my $subject;
    my $type;
    my $to;
    my $ret = GetOptionsFromArray(\@_,
                               "c:s" => \$stream,
                               "i:s" => \$subject);
    unless($ret) {
        die("Usage: zulip:write [-c stream] [-i subject] [recipient] ...");
    }
    # anything left is a recipient
    if (scalar(@_) > 0) {
        my @addresses = map {
            if(/@/) {
                $_;
            } else {
                $_ . "\@$cfg{'realm'}";
            }} @_;
        $to = encode_json(\@addresses);
        $type = "private";
            
    } else {
        $type = "stream";
        $to = $stream
    }
    BarnOwl::start_edit(prompt => $cmdline, type => 'edit_win', 
                        callback => sub {
                            my ($text, $should_send) = @_;
                            unless ($should_send) {
                                BarnOwl::message("zulip:write cancelled");
                                return;
                            }
                            zulip($type, $to, $subject, $text);
                        });
    
}

BarnOwl::new_command('zulip:login' => sub { cmd_zulip_login(@_); },
                     {
                         summary => "Log in to Zulip",
                         usage => "zulip:login",
                         description => "Start receiving Zulip messages"
                     });

BarnOwl::new_command('zulip:write' => sub { cmd_zulip_write(@_); },
                     {
                         summary => "Send a zulipgram",
                         usage => "zulip:login [-c stream] [-i subject] [recipient(s)]",
                         description => "Send a zulipgram to a stream, person, or set of people"
                     });

BarnOwl::new_command('zulip:subscribe' => sub { cmd_zulip_sub(@_); },
                     {
                         summary => "Subscribe to a Zulip stream",
                         usage => "zulip:subscribe <stream name>",
                         description => "Subscribe to a Zulip stream"
                     });

BarnOwl::new_command('zulip:unsubscribe' => sub { cmd_zulip_unsub(@_); },
                     {
                         summary => "Unsubscribe from a Zulip stream",
                         usage => "zulip:unsubscribe <stream name>",
                         description => "Unsubscribe to a Zulip stream"
                     });

BarnOwl::new_command('zulip:getsubs' => sub { cmd_zulip_getsubs(@_); },
                     {
                         summary => "Display the list of subscribed Zulip streams",
                         usage => "zulip:getsubs",
                         description => "Display the list of Zulip streams you're subscribed to in a popup window"
                     });


sub user {
  return $cfg{'user'};
}

sub default_realm {
  return $cfg{'realm'};
}

initialize();

1;

# Local Variables:
# indent-tabs-mode: nil
# End:
