#!/usr/bin/env perl

use warnings;
use strict;
use feature 'say';

use WWW::Mechanize;

#my $uri = 'http://localhost:55556';
my $uri = 'https://tesla.hellbent.app:55556/garage_door_operate';

my $m = WWW::Mechanize->new;

my $content = {
    token => ''
};

my $ret = $m->post($uri, $content);

say $ret->decoded_content;
