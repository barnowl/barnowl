use strict;
use warnings;

package BarnOwl::Message::Zulip;
use base qw(BarnOwl::Message);

use BarnOwl::Module::Zulip qw(default_realm);


sub strip_realm {
    my $sender = shift;
    my $realm = BarnOwl::Module::Zulip::default_realm();
    $sender =~ s/\@\Q$realm\E$//;
    return $sender;
}

sub context {
    return shift->class;
}

sub subcontext {
    return shift->instance;
}

sub pretty_sender {
    my ($m) = @_;
    return strip_realm($m->sender);
}

sub pretty_recipient {
    my ($m) = @_;
    return strip_realm($m->recipient);
}

sub private_recipient_list {
    my ($m) = @_;
    return split(/ /, $m->recipient);
}

sub class       { return shift->{"class"}; }
sub instance    { return shift->{"instance"}; }
sub realm       { return shift->{"realm"}; }
sub opcode       { return shift->{"opcode"}; }
sub long_sender        { return shift->{"sender_full_name"}; }
sub zid { return shift->{zid}; }

sub replycmd {
    my $self = shift;
    if ($self->is_private) {
        return $self->replyprivate(1);
    } else {
        return BarnOwl::quote("zulip:write", "-c", $self->class,
                              "-i", $self->instance);
    }
}

sub replysendercmd {
    my $self = shift;
    if ($self->is_private) {
        return $self->replyprivate(0);
    } else {
        return BarnOwl::quote("zulip:write", $self->sender);
    }
}

sub replyprivate {
    # Cases:
    # - me -> me: me
    # - me -> other: other
    # - (all) me -> other1, other2: other1, other2
    # - (not) me -> other1, other2: error
    # - other -> me: other
    # - (all) other1 -> me, other2: other1, other2
    # - (not) other1 -> me, other2: other1
    my $self = shift;
    my $all = shift;
    my @recipients;
    if ($all) {
       @recipients = $self->private_recipient_list;
    } else {
        if ($self->sender eq BarnOwl::Module::Zulip::user()) {
            @recipients = $self->private_recipient_list;
            if (scalar(@recipients) > 1) {
                BarnOwl::error("Cannot reply privately to message to more than one recipient");
                return;
            }
        } else {
            @recipients = $self->sender;
        }
    }
    my @filtered_recipients = grep { $_ ne BarnOwl::Module::Zulip::user() } @recipients;
    if (scalar(@filtered_recipients) == 0) {
        # Self must have been only recipient, so re-add it (one copy)
        @filtered_recipients = @recipients[0];
    }
    return BarnOwl::quote("zulip:write", @filtered_recipients);
}

# Strip layers of "un" and ".d" from names
sub base_name {
    my $name = shift;

    while($name =~ /(^un)/) {
	$name =~ s/(^un)//;
    }
    while($name =~ /\.d$/) {
	$name =~ s/(\.d$)//g;
    }
    return $name;
}

sub baseclass {
    my $self = shift;
    return base_name($self->class);
}

sub baseinstance {
    my $self = shift;
    return base_name($self->instance);
}

sub quote_for_filter {
    my $quote_chars = '!+*.?\[\]\^\\${}()|';
    my $str = shift;
    return ($str =~ s/[$quote_chars]/\\$1/gr);
}

# This is intended to be a port of the Zephyr part of
# owl_function_smartfilter from functions.c.
sub smartfilter {
    my ($self, $inst, $related) = @_;
    my $filter;
    my @filter;
    if($self->is_private) {
	my @recips = $self->private_recipient_list;
	if(scalar(@recips) > 1) {
	    BarnOwl::message("Smartnarrow for group personals not supported yet. Sorry :(");
	    return "";
	} else {
	    my $person;
	    if($self->direction eq "in") {
		$person = $self->sender;
	    } else {
		$person = $self->recipient;
	    }
	    $filter = "zulip-user-$person";
	    #	    $person =~ s/\./\\./
	    $person =~ quote_for_filter($person);
	    @filter = split / /, "( type ^Zulip\$ and filter personal and ( ( direction ^in\$ and sender ^${person}\$ ) or ( direction ^out\$ and recipient ^${person}\$ ) ) )";
	}
    } else {
	my $class;
	my $instance;

	if($related) {
	    $class = $self->baseclass;
	    if($inst) {
		$instance = $self->baseinstance;
	    }
	    $filter = "related-";
	} else {
	    $class = $self->class;
	    if($inst) {
		$instance = $self->instance;
	    }
	    $filter = "";
	}
	$filter .= "class-$class";
	if($inst) {
	    $filter .= "-instance-$instance";
	}
	$class = BarnOwl::quote(quote_for_filter($class));
	# TK deal with filter already existing
	my $filter_str = "";
	$filter_str .= ($related ? "class ^(un)*${class}(\\.d)*\$" : "class ^${class}\$");
	if($inst) {
	    $instance = BarnOwl::quote(quote_for_filter($instance));
	    $filter_str .= ($related ? " and ( instance ^(un)*${instance}(\\.d)*\$ )" : " and ( instance ^${instance}\$ )");
	}
	@filter = split / /, $filter_str;
    }
    BarnOwl::command("filter", $filter, @filter);
    return $filter;
}

1;
