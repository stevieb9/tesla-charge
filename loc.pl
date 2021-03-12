use warnings;
use strict;
use feature 'say';


use Data::Dumper;
use JSON;

use constant {
    DEBUG       => 0,
    ACCURACY    => 1e4,
    RANGE       => 0.5,
    LAT         => 50.25892,
    LON         => -119.3166,
};


my $data = `python3 tesla.py`;
$data = decode_json $data;

if (! @ARGV && $data->{state} ne 'online') {
    print "Vehicle is offline\n";
    exit;
}

$data = `python3 tesla.py 1`;
$data = decode_json $data;

my $chg         = $data->{charge_state}{battery_level};
my $charging    = $data->{charge_state}{charging_state};
my $lat         = $data->{drive_state}{latitude};
my $lon         = $data->{drive_state}{longitude};
my $gear        = $data->{drive_state}{shift_state};

$charging = $charging eq 'Disconnected' ? 0 : 1;

if (DEBUG) {
    say "Data: $data";
    say "chg: $chg, gear: $gear, lat: $lat, lon: $lon";
    printf("Lat: %f, Lon: %f\n", distance('lat', $lat), distance('lon', $lon));
}

my %out_of_bounds;

if (! deviation('lat', $lat)) {
    $out_of_bounds{Latitude} = distance('lat', $lat);
}
if (! deviation('lon', $lon)) {
    $out_of_bounds{Longitude} = distance('lon', $lon);
}

if (keys %out_of_bounds) {
    for (keys %out_of_bounds) {
        say "$_ out of bounds by: $out_of_bounds{$_}";
    }
    say "Car is not in garage. Gear: $gear Charge: $chg Charging: $charging";
    exit;
}

say "Car is in the garage. Charge: $chg Gear: $gear Charging: $charging";

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
