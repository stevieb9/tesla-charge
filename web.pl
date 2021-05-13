#!/usr/bin/env perl

# Crontab entry

# @reboot sleep 10; cd /home/pi/repos/tesla-charge; /home/pi/perl5/perlbrew/perls/perl-5.30.1/bin/perl /home/pi/repos/tesla-charge/web.pl > /tmp/tesla_web.log 2>&1

use warnings;
use strict;
use 5.10.0;

use Async::Event::Interval;
use Dancer2;
use Data::Dumper;
use Data::Dumper;
use FindBin;
use IPC::Shareable;
use JSON;

$| = 1;

set port        => 55556;
#set serializer => 'JSON';

use constant {
    ACCURACY    => 1e4,
    RANGE       => 1.2,
    LAT         => 50.25892,
    LON         => -119.3166,
    CONFIG_JSON => "$FindBin::Bin/config.json",
    DATA_EXPIRY => 10, # Seconds
};

my $tesla_conf;
my $tesla_debug = 0;

my $garage_conf;
my $garage_door_open    = 0;
my $garage_door_action  = 0;
my $garage_door_manual  = 0;
my $garage_debug        = 0;

tie my $data, 'IPC::Shareable', 'TSLA', {create => 1, destroy => 1};
$data = '';

my $last_conn_time = time;

my $tesla_event = Async::Event::Interval->new(0, \&update);
$tesla_event->start;

get '/' => sub {
    return if ! security();

    content_type 'application/json';
    
    config_load();
    
    return debug_data() if $tesla_conf->{debug_return};

    if (time - $last_conn_time > DATA_EXPIRY) { 
        $data = '';
    }

    $last_conn_time = time;

    $tesla_event->start if $tesla_event->waiting;
    
    return $data if $data;
    return encode_json _default_data();
};

get '/debug' => sub {
    return if ! security();
   
    content_type 'application/json';
    config_load();
    return debug_data();
};

get '/wake' => sub {
    return if ! security();

    my $data = `python3 /home/pi/repos/tesla-charge/wake.py`;
    return $data;
};

# Main page (web)

get '/garage' => sub {
    return if ! security();
    return template 'garage';
};

# Get garage data (microcontroller)

get '/garage_data' => sub {
    return if ! security();

    content_type 'application/json';
    
    config_load();
    
    $tesla_event->start if $tesla_event->waiting;
   
    my %garage_data =  %{ $garage_conf };

    if ($data) {
        my $data_ref = decode_json $data;
        $garage_data{garage} = $data_ref->{garage};
    }
    else {
        $garage_data{garage} = -1;
    }

    return encode_json \%garage_data;
};

# Get door state

get '/garage_door_state' => sub {
    return if ! security();
    return int $garage_door_open;
};

# Set door state (microcontroller JSON)

post '/garage_door_state_set' => sub {
    return if ! security();
    
    my $data = decode_json request->body;
    $garage_door_open = $data->{open};
    return;
};

# Toggle door state (web)

get '/garage_door_toggle' => sub {
    return if ! security();
    if ($garage_door_open) {
        $garage_door_open = 0;
    }
    else {
        $garage_door_open = 1;
    }
};

# Get door action pending

get '/garage_door_action' => sub {
    return if ! security();
    return $garage_door_action;
};

# Fetch and reset door action pending (microcontroller)

get '/garage_door_action_get' => sub {
    return if ! security();

    my $action = $garage_door_action;
    $garage_door_action = 0;
    return $action;
};

# Set door action (web)

post '/garage_door_action_set' => sub {
    return if ! security();
    $garage_door_action = 1;
    return;
};

# Get door manual mode

get '/garage_door_manual' => sub {
    return if ! security();
    return $garage_door_manual;
};

# Set garage door manual mode (web)

get '/garage_door_manual_set' => sub {
    return if ! security();

    if ($garage_door_manual) {
        $garage_door_manual = 0;
    }
    else {
        $garage_door_manual = 1;
    }

    return;
};

dance;

sub security {
    return if request->address !~ /^192\.168\.0\.\d+/;
    return 1;
}
sub config_load {
    my $conf;

    {
        local $/;
        open my $fh, '<', CONFIG_JSON or die "Can't open config JSON file: $!";
        $conf = <$fh>;
    }

    $conf = decode_json $conf;
    
    $tesla_conf  = $conf->{tesla_vehicle};
    $garage_conf = $conf->{garage_door}; 
    
    $tesla_debug = 1 if $tesla_conf->{debug};
    $garage_debug = 1 if $garage_conf->{debug};
}
sub debug_data {
    return encode_json $tesla_conf->{debug_data};
}
sub update {
    my $local_data = -1;

    until ($local_data != -1) {
        $local_data = fetch($tesla_conf);
        if ($local_data->{error} && $tesla_conf->{retry}) {
            for (0..$tesla_conf->{retry} - 1) {
                $local_data = fetch($tesla_conf);
                last if ! $local_data->{error};
            }
        }
        $data = encode_json $local_data;
    }
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
        fetching    => 0
    };

    if ($conf->{rainbow}) {
        print "Rainbow!\n" if $tesla_debug;
        $struct->{rainbow} = 1;
        return $struct;
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
        print "Offline!\n" if $tesla_debug;
        $struct->{online} = 0; 
        return $struct;
    }
    else {
        $chg        = $data->{charge_state}{battery_level};
        $charging   = $data->{charge_state}{charging_state};
        my $lat     = $data->{drive_state}{latitude};
        my $lon     = $data->{drive_state}{longitude};
        $gear       = $data->{drive_state}{shift_state};

        $charging //= 'Disconnected';
        $charging = $charging eq 'Disconnected' ? 0 : 1;

        if (! defined $gear || $data->{'request-error'}) {
            print "Error: Corrupt JSON data from Tesla API.\n";
            $struct->{error} = 1;
            return $struct;
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

        return $struct;
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
sub _default_data {
    my $struct = {
        online      => 0, # Vehicle is online
        garage      => 0, # Vehicle is in the garage
        charge      => 0, # Vehicle charge percent
        charging    => 0, # Vehicle is being charged
        gear        => 0, # Park: 0, Reverse: 1, Drive: 2
        error       => 0, # Tesla API error
        rainbow     => 0, # Rainbow LED mode
        fetching    => 1, # Currently fetching data from Tesla
    };

    return $struct;
}
sub _default_garage_data {
    my %data = %{ $garage_conf };
    $data{garage} = -1;
    return \%data;
}
