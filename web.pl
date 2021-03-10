#!/usr/bin/env perl

use warnings;
use strict;

use Dancer2;
use Data::Dumper;
use JSON;

use constant {
    DEBUG       => 0,
    ACCURACY    => 1e4,
    RANGE       => 1.2,
    LAT         => 50.25892,
    LON         => -119.3166,
};

get '/' => sub {
    content_type 'application/json';
    return fetch();
};

start;

sub fetch {
    my $data = decode_json `python3 tesla.py`;

    my ($state, $chg, $charging, $gear);

    my $online = $data->{state} eq 'online' ? 1 : 0;

    if (! $online) {
        print "Offline!\n";
        
        return encode_json {
            online      => $online,
            garage      => 0,
            charge      => 0,
            charging    => 0,
            gear        => 0,
        };
    }
    else {
    
        $data = decode_json `python3 tesla.py 1`;

        $chg        = $data->{charge_state}{battery_level};
        $charging   = $data->{charge_state}{charging_state};
        my $lat     = $data->{drive_state}{latitude};
        my $lon     = $data->{drive_state}{longitude};
        $gear       = $data->{drive_state}{shift_state};

        $charging = $charging eq 'Disconnected' ? 0 : 1;
        
        $gear = 0 if $gear eq 'P';
        $gear = 1 if $gear eq 'R';
        $gear = 2 if $gear eq 'D';

        my %out_of_bounds;

        if (! deviation('lat', $lat)) {
            $out_of_bounds{Latitude} = distance('lat', $lat);
        }
        if (! deviation('lon', $lon)) {
            $out_of_bounds{Longitude} = distance('lon', $lon);
        }

        my $garage = keys %out_of_bounds ? 0 : 1;

        my $json_data = {
            online      => $online,
            charge      => $chg,
            charging    => $charging,
            garage      => $garage,
            gear        => int $gear,
        };

        return encode_json $json_data;
    }
}
sub deviation {
    die "Need lat|lon and coord" if @_ != 2;
    
    my ($what, $coord) = @_;

    $what eq 'lat'
        ? return abs(ACCURACY * (LAT - $coord)) < 1
        : return abs(ACCURACY * (LON - $coord)) < 1;
}
sub distance {
    die "Need lat|lon and coord" if @_ != 2;
    
    my ($what, $coord) = @_;

    $what eq 'lat'
        ? return abs(ACCURACY * (LAT - $coord))
        : return abs(ACCURACY * (LON - $coord));
}
