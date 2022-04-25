#!/usr/bin/env perl

# Generates user access tokens for the web API

use warnings;
use strict;

use FindBin qw($RealBin);
use Digest::SHA qw(sha256_hex);
use JSON;
use MIME::Base64 qw(encode_base64url);

print "\n\nEnter a name to associate with this token: ";
my $name = <>;
chomp $name;

my $token = generate_token();

print "\nAdd the following to the 'tokens' section of the 'config.json' file:\n\n";

printf '"%s": "%s"', $name, $token;

print "\n\n";

sub generate_token {
    my $token_string = _random_string();
    my $token_hex = sha256_hex($token_string);
    my $token = encode_base64url($token_hex);

    return $token;
}
sub _random_string {
    my @chars = ('A' .. 'Z', 'a' .. 'z', 0 .. 9);
    my $rand_string;
    $rand_string .= $chars[rand @chars] for 1 .. 85;
    return $rand_string;
}