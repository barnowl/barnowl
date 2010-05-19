#!/bin/sh
SRCDIR=`dirname "${0}"`

export BARNOWL_DATA_DIR="$SRCDIR/perl/"
export BARNOWL_BIN_DIR="$SRCDIR/"

HARNESS_PERL=./tester exec prove t/
