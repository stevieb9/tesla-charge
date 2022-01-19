#!/usr/bin/env perl

# Crontab entry
# @reboot sleep 10; cd /home/pi/repos/tesla-charge; /home/pi/perl5/perlbrew/perls/perl-5.30.1/bin/perl /home/pi/repos/tesla-charge/web.pl > /tmp/tesla_web.log 2>&1

# Auto-reloading plack command
# plackup --access-log /dev/null -p 55556 -r -R . web.pl

use warnings;
use strict;
use 5.10.0;

use Async::Event::Interval;
use Dancer2;
use Data::Dumper;
use FindBin;
use IPC::Shareable;

$| = 1;

set port => 55556;

use constant {
    ACCURACY        => 1e4,
    RANGE           => 1.2,
    LAT             => 50.25892,
    LON             => -119.3166,
    GARAGE_OPEN     => 1,
    GARAGE_CLOSED   => 0,
    CONFIG_JSON     => "$FindBin::Bin/config.json",
    DATA_EXPIRY     => 10, # Seconds
};

my $system_conf;
my $tesla_conf;
my $garage_conf;

my $tesla_debug = 0;
my $garage_debug = 0;

config_load();

# Tesla data
tie my $tesla_data, 'IPC::Shareable', 'TSLA', {create => 1, destroy => 1};
$tesla_data = '';

# Garage data
my $garage_data = _default_garage_data();

my $tesla_event = Async::Event::Interval->new(0, \&update);
$tesla_event->start;

my $last_conn_time = time;

get '/' => sub {
    return if ! security();

    content_type 'application/json';

    config_load();

    return debug_data() if $tesla_conf->{debug_return};

    if (time - $last_conn_time > DATA_EXPIRY) { 
        $tesla_data = '';
    }

    $last_conn_time = time;

    $tesla_event->start if $tesla_event->waiting;
    
    return $tesla_data if $tesla_data;
    return encode_json _default_data();
};
get '/debug' => sub {
    return if ! security();
   
    content_type 'application/json';
    config_load();
    return debug_data();
};
get '/debug_garage' => sub {
    return if ! security();

    content_type 'application/json';
    config_load();
    return debug_garage_data();
};
get '/wake' => sub {
    # Wake up the Tesla
    return if ! security();

    my $data = `python3 $system_conf->{script_path}/wake.py`;

    if ($data == -1) {
        return "<html><body>Error code -1: Failed to wake the car</body></html>";
    }

    redirect '/';
};
get '/garage' => sub {
    # Main garage page (web)
    return if ! security();
    return template 'garage';
};
get '/garage_data' => sub {
    # Get garage data (microcontroller)
    return if ! security();

    content_type 'application/json';
    
    config_load();

    $garage_data->{relay_enabled} = $garage_conf->{relay_enabled};
    $garage_data->{app_enabled} = $garage_conf->{app_enabled};
    $garage_data->{auto_close_enabled} = $garage_conf->{auto_close_enabled};

    return encode_json $garage_data if $garage_data;
};
get '/garage_door_state' => sub {
    # Get garage door state
    return if ! security();
    return int $garage_data->{garage_door_state};
};
post '/garage_update' => sub {
    # Update garage data (microcontroller JSON)
    return if ! security();

    my $data = decode_json request->body;

    $garage_data->{garage_door_state} = $data->{door_state};
    $garage_data->{activity} = $data->{activity};

    return;
};
get '/garage_door_toggle' => sub {
    # Toggle door state (web)
    return if ! security();
};

dance;

sub security {
    if ($system_conf->{secure_ip}) {
        return if request->address !~ /^192\.168\.1\.\d+/;
    }
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

    $system_conf = $conf->{system};
    $tesla_conf  = $conf->{tesla_vehicle};
    $garage_conf = $conf->{garage};

    $tesla_debug = 1 if $tesla_conf->{debug};
    $garage_debug = 1 if $garage_conf->{debug};
}
sub debug_data {
    return encode_json $tesla_conf->{debug_data};
}
sub debug_garage_data {
    return encode_json $garage_conf->{debug_data};
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
        $tesla_data = encode_json $local_data;
    }
}
sub fetch {
    my ($conf) = @_;
    my ($online, $chg, $charging, $gear);

    my $struct = _default_data();
    $struct->{fetching} = 0;

    if ($conf->{rainbow}) {
        print "Rainbow!\n" if $tesla_debug;
        $struct->{rainbow} = 1;
        return $struct;
    }

    my $data = `python3 $system_conf->{script_path}/tesla.py`;

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
    my $struct = {
        garage_door_state   => -1,  # 0 = Closed, 1 = Open, 2 = Opening, 3 = Closing, -1 = Unknown
        tesla_in_garage     => -1,  # 1 = Tesla in garage, 0 = Tesla not in garage
        activity            => 0,   # 0 = None, 1 = Door
        relay_enabled       => 0,   # Bool: Enable/disable the door relay
        app_enabled         => 0,   # Bool: Disable front-end API
        auto_close_enabled  => 0,   # Bool: Enable/disable garage door auto-close
    };

    return $struct;
}