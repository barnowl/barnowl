#!/bin/sh
exec env HARNESS_PERL=./tester prove --failures t/
