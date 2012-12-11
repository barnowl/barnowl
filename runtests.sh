#!/bin/sh
exec env HARNESS_PERL=./tester prove --failures "${srcdir:=$(dirname "$0")}/t/"
