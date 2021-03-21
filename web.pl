#!/usr/bin/env perl

use warnings;
use strict;
use 5.10.0;

use Dancer2;
use Data::Dumper;
use FindBin;
use JSON;

set port        => 55556;

use constant {
    DEBUG       => 0,
    ACCURACY    => 1e4,
    RANGE       => 1.2,
    LAT         => 50.25892,
    LON         => -119.3166,
    CONFIG_JSON  => "$FindBin::Bin/config.json",
};


get '/' => sub {
    my $conf = config();
    
    content_type 'application/json';
    
    my $data = -1;

    until ($data ne 'request-error' && $data != -1) {
        $data = fetch($conf);
        return $data if $data ne 'request-error';

    }
};

get '/debug' => sub {
    my $conf = config();
    content_type 'application/json';
    return debug_data($conf);
};

start;

sub config {
    my $conf;

    {
        local $/;
        open my $fh, '<', CONFIG_JSON or die "Can't open config JSON file: $!";
        $conf = <$fh>;
    }

    return decode_json $conf;
}
sub debug_data {
    my ($conf) = @_;
    return encode_json $conf->{debug_data};
}
sub fetch {
    my ($conf) = @_;
    my ($online, $chg, $charging, $gear);

    my $struct = {
        online      => 0,
        garage      => 0,
        charge      => 0,
        charging    => 0,
        gear        => 0,
        error       => 0,
        rainbow     => 0,
    };

    if ($conf->{rainbow}) {
        print "Rainbow!\n" if DEBUG;
        $struct->{rainbow} = 1;
        return encode_json $struct;
    }

    my $data = `python3 /home/pi/repos/tesla-charge/tesla.py`;
    $data = decode_json $data; 

    if (defined $data->{online} && ! $data->{online}) {
        $online = 0;
    }
    else {
        $online = 1;
    }
    
    if (! $online) {
        print "Offline!\n" if DEBUG;
        $struct->{online} = 0; 
        return encode_json $struct;
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
            print "Error: Corrupt JSON data from Tesla API.\n";
            $struct->{error} = 1;
            return encode_json $struct;
        }
       
        $gear = gear($gear);

        my %out_of_bounds;

        if (! deviation('lat', $lat)) {
            $out_of_bounds{Latitude} = distance('lat', $lat);
        }
        if (! deviation('lon', $lon)) {
            $out_of_bounds{Longitude} = distance('lon', $lon);
        }

        my $garage = keys %out_of_bounds ? 0 : 1;

        $struct->{online}       = int $online;
        $struct->{charge}       = int $chg;
        $struct->{charging}     = int $charging;
        $struct->{garage}       = int $garage;
        $struct->{gear}         = int $gear;

        return encode_json $struct;
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
sub gear {
    my ($gear) = @_;
    return 0 if $gear eq 'P';
    return 1 if $gear eq 'R';
    return 2 if $gear eq 'D';
    return 2 if $gear eq 'N';
}
