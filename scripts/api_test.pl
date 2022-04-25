#!/usr/bin/env perl

use warnings;
use strict;

use WWW::Mechanize;

my $uri = 'http://localhost:55556';

my $m = WWW::Mechanize->new;

my $content = { token => 'MzM3MWFlZDMyODViMDJmZGIxZjgwMzE3MmNkYWRhMzlmNjEyZTRhZDFkMjhmMTA5OGZiM2Y3ZWQ0YzkzYTEwYw' };
$m->post($uri, $content);

$m->get($uri);

