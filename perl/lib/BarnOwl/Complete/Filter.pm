use strict;
use warnings;

# Helper completers for filters
package BarnOwl::Complete::Filter;

use base qw(Exporter);
our @EXPORT_OK = qw(complete_filter_name complete_filter_expr);

# Completes the name of a filter
sub complete_filter_name { return @{BarnOwl::all_filters()}; }

# Completes a filter expression
sub complete_filter_expr {
    my $ctx = shift;

    my @completions = ();
    _complete_filter_expr($ctx, 0, \@completions);
    # Get rid of duplicates and sort
    my %hash = ();
    @hash{@completions} = ();
    @completions = sort keys %hash;
    return @completions;
}

### Private

# Exported for the tester
our %filter_cmds = (
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
    deleted   => sub { qw(true false); },
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
    return $INCOMPLETE if $i == $INCOMPLETE;

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
