#!/usr/bin/env perl

use warnings;
use strict;
use 5.10.0;

use Dancer2;
use Data::Dumper;
use JSON;

set port         => 55556;

use constant {
    DEBUG       => 0,
    ACCURACY    => 1e4,
    RANGE       => 1.2,
    LAT         => 50.25892,
    LON         => -119.3166,
}; 
get '/' => sub {
    content_type 'application/json';

    my $data = -1;

    until ($data ne 'request-error' && $data != -1) {
        $data = fetch();
        return $data if $data ne 'request-error';

    }
};

start;

sub fetch {
    my ($online, $chg, $charging, $gear);

    my $data = `python3 /home/pi/repos/tesla-charge/tesla.py`;
    $data = decode_json $data; 

    if (defined $data->{online} && ! $data->{online}) {
        $online = 0;
    }
    else {
        $online = 1;
    }
    
    if (! $online) {
        print "Offline!\n";
        
        return encode_json {
            online      => int $online,
            garage      => 0,
            charge      => 0,
            charging    => 0,
            gear        => 0,
            error       => 0,
        };
    }
    else {
        $chg        = $data->{charge_state}{battery_level};
        $charging   = $data->{charge_state}{charging_state};
        my $lat     = $data->{drive_state}{latitude};
        my $lon     = $data->{drive_state}{longitude};
        $gear       = $data->{drive_state}{shift_state};

        $charging //= 'Disconnected';
        $charging = $charging eq 'Disconnected' ? 0 : 1;

        if (! defined $gear) {
            print "No gear!\n";
            return encode_json {
                online      => 0,
                garage      => 0,
                charge      => 0,
                charging    => 0,
                gear        => 0,
                error       => 1,
            };
        }
       
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
            online      => int $online,
            charge      => int $chg,
            charging    => int $charging,
            garage      => int $garage,
            gear        => int $gear,
            error       => 0,
        };

        my $json = encode_json $json_data;
        return $json;
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
