use strict;
use warnings;

# Completers for client state

package BarnOwl::Complete::Client;

use BarnOwl::Completion::Util qw(complete_flags);

my @all_colors = qw(default
                    black
                    blue
                    cyan
                    green
                    magenta
                    red
                    white
                    yellow);

my %show = (
    information => undef,
    colors      => undef,
    commands    => undef,
    command     => \&complete_command,
    filters     => undef,
    filter      => \&complete_filter_name,
    license     => undef,
    quickstart  => undef,
    startup     => undef,
    status      => undef,
    styles      => undef,
    subscriptions       => undef,
    subs        => undef,
    terminal    => undef,
    variables   => undef,
    variable    => \&complete_variable,
    version     => undef,
    view        => undef,
    zpunts      => undef,
   );

sub complete_command { return sort @BarnOwl::all_commands; }
sub complete_color { return @all_colors; }
sub complete_filter_name { return @{BarnOwl::all_filters()}; }
sub complete_variable {}        # stub

my %filter_cmds = (
    sender    => \&BarnOwl::Complete::Zephyr::complete_user,
    recipient => \&BarnOwl::Complete::Zephyr::complete_user,
    class     => \&BarnOwl::Complete::Zephyr::complete_class,
    instance  => \&BarnOwl::Complete::Zephyr::complete_instance,
    opcode    => \&BarnOwl::Complete::Zephyr::complete_opcode,
    realm     => \&BarnOwl::Complete::Zephyr::complete_realm,
    body      => undef,
    hostname  => undef,
    type      => sub { qw(zephyr aim admin); },
    direction => sub { qw(in out none); },
    login     => sub { qw(login logout none); },
    filter    => \&complete_filter_name,
    perl      => undef,
);

# Returns:
# - where to look next after pulling out an expression
# - $INCOMPLETE if this cannot form a complete expression (or w/e)
# - pushes to completion list as it finds valid completions

my $INCOMPLETE = -1;
sub _complete_filter_expr {
    # Takes as arguments context and the index into $ctx->words where the
    # filter expression starts
    my $ctx = shift;
    my $start = shift;
    my $o_comp = shift;
    my $end = $ctx->word;

    # Grab an expression; we don't allow empty
    my $i = $start;
    $i = _complete_filter_primitive_expr($ctx, $start, $o_comp);
    return $INCOMPLETE if $start == $INCOMPLETE;

    while ($i <= $end) {
        if ($i == $end) {
            # We could and/or another clause
            push @$o_comp, qw(and or);
            return $end; # Or we could let the parent do his thing
        }

        if ($ctx->words->[$i] ne 'and' && $ctx->words->[$i] ne 'or') {
            return $i; # We're done. Let the parent do his thing
        }

        # Eat the and/or
        $i++;

        # Grab an expression
        $i = _complete_filter_primitive_expr($ctx, $i, $o_comp);
        return $INCOMPLETE if $i == $INCOMPLETE;
    }
    
    return $i; # Well, it looks like we're happy
    # (Actually, I'm pretty sure this never happens...)
}

sub _complete_filter_primitive_expr {
    my $ctx = shift;
    my $start = shift;
    my $o_comp = shift;
    my $end = $ctx->word;

    if ($start >= $end) {
        push @$o_comp, "(";
        push @$o_comp, qw(true false not);
        push @$o_comp, keys %filter_cmds;
        return $INCOMPLETE;
    }

    my $word = $ctx->words->[$start];
    if ($word eq "(") {
        $start = _complete_filter_expr($ctx, $start+1, $o_comp);
        return $INCOMPLETE if $start == $INCOMPLETE;

        # Now, we expect a ")"
        if ($start >= $end) {
            push @$o_comp, ")";
            return $INCOMPLETE;
        }
        if ($ctx->words->[$start] ne ')') {
            # User is being confusing. Give up.
            return $INCOMPLETE;
        }
        return $start+1; # Eat the )
    } elsif ($word eq "not") {
        # We just want another primitive expression
        return _complete_filter_primitive_expr($ctx, $start+1, $o_comp);
    } elsif ($word eq "true" || $word eq "false") {
        # No arguments
        return $start+1; # Eat the boolean. Mmmm, tasty.
    } else {
        # It's of the form 'CMD ARG'
        return $start+2 if ($start+1 < $end); # The user supplied the argument

        # complete the argument
        my $arg_func = $filter_cmds{$word};
        push @$o_comp, ($arg_func ? ($arg_func->()) : ());
        return $INCOMPLETE;
    }
}

sub complete_filter_expr {
    my $ctx = shift;
    my $start = shift;
    my @completions = ();
    _complete_filter_expr($ctx, $start, \@completions);
    # Get rid of duplicates and sort
    my %hash = ();
    @hash{@completions} = ();
    @completions = sort keys %hash;
    return @completions;
}

sub complete_filter_args {
    my $ctx = shift;
    my $arg = shift;
    return complete_filter_name() unless $arg;
    my $idx = 2; # skip the filter name
    while ($idx < $ctx->word) {
        last unless ($ctx->words->[$idx] =~ m{^-});
        $idx += 2; # skip the flag and the argument
    }
    return complete_filter_expr($ctx, $idx);
}

sub complete_help {
    my $ctx = shift;
    if($ctx->word == 1) {
        return complete_command();
    }
}

sub complete_show {
    my $ctx = shift;
    if($ctx->word == 1) {
        return keys %show;
    } elsif ($ctx->word == 2) {
        my $cmd = $show{$ctx->words->[1]};
        if($cmd) {
            return $cmd->($ctx);
        }
    }
}

sub complete_filter {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw()],
        {
           "-c" => \&complete_color,
           "-b" => \&complete_color,
        },
         \&complete_filter_args
        );
}

sub complete_view {
    my $ctx = shift;
    if ($ctx->word == 1) {
        return ("--home", "-d", complete_filter_name());
    }
    if ($ctx->words->[1] eq "--home") {
        return;
    }
    if ($ctx->words->[1] eq "-d") {
        return complete_filter_expr($ctx, 2);
    }
    return;
}

sub complete_getvar {
    my $ctx = shift;
    return unless ($ctx->word == 1);
    return complete_variable();
}

sub complete_set {
    my $ctx = shift;
    return complete_flags($ctx,
        [qw(-q)],
        {
        },
         \&complete_set_args
        );
}
sub complete_set_args {
    my $ctx = shift;
    my $arg = shift;
    return if $arg;
    return complete_variable();
}

BarnOwl::Completion::register_completer(help    => \&complete_help);
BarnOwl::Completion::register_completer(filter  => \&complete_filter);
BarnOwl::Completion::register_completer(view    => \&complete_view);
BarnOwl::Completion::register_completer(show    => \&complete_show);
BarnOwl::Completion::register_completer(getvar  => \&complete_getvar);
BarnOwl::Completion::register_completer(set     => \&complete_set);

1;
