#!/usr/bin/env perl

use warnings;
use strict;
use feature 'say';

use WWW::Mechanize;

my $uri = 'https://tesla.hellbent.app:55556';

my $m = WWW::Mechanize->new;

my $content = { token => 'NTE2M2Q3YmFmMGEzNjEzMGQyZmFmYmViNGE4OTZmNzkzMTE3NzI3MWMzMDhkZjliYWE3NTI1OTg1MDQwNjNlZg' };

my $ret = $m->post($uri, $content);

say $ret->decoded_content;

#$m->get($uri);

