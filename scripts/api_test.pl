#!/usr/bin/env perl

use warnings;
use strict;
use feature 'say';

use WWW::Mechanize;

my $uri = 'https://tesla.hellbent.app:55556';

my $m = WWW::Mechanize->new;

my $content = { token => 'MzM3MWFlZDMyODViMDJmZGIxZjgwMzE3MmNkYWRhMzlmNjEyZTRhZDFkMjhmMTA5OGZiM2Y3ZWQ0YzkzYTEwYw' };

my $ret = $m->post($uri, $content);

say $ret->decoded_content;

#$m->get($uri);

