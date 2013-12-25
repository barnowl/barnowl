#!/bin/sh
export G_SLICE=debug-blocks
exec env HARNESS_PERL=./tester prove --failures "${srcdir:=$(dirname "$0")}/t/"
