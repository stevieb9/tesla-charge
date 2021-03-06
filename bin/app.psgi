#!/usr/bin/env perl

use warnings;
use strict;
use 5.10.0;

use Async::Event::Interval;
use Dancer2;
use Data::Dumper;
use FindBin;
use Net::Subnet;
use IPC::Shareable;
use Tesla::Vehicle;
use Time::Piece qw(localtime);

our $VERSION = '1.02';

$| = 1;

set port => 55556;

use constant {
    ACCURACY        => 1e4,
    RANGE           => 1.2,
    GARAGE_OPEN     => 1,
    GARAGE_CLOSED   => 0,
    CONFIG_JSON     => "$FindBin::Bin/../config/config.json",
    DATA_EXPIRY     => 10, # Seconds
};

# Configuration stores
my $system_conf;
my $tesla_conf;
my $garage_conf;

# Debug flags
my $tesla_debug = 0;
my $garage_debug = 0;

# Initial load of configuration file
config_load();

# Vehicle location
my $latitude  = $tesla_conf->{latitude} || 0;
my $longitude = $tesla_conf->{longitude} || 0;

# Tesla vehicle API
my $car = Tesla::Vehicle->new(api_cache_persist => 1);

# Tesla data
tie my $tesla_data, 'IPC::Shareable', {key => 'tesla_charge', create => 1};
$tesla_data = '';

# Garage data
my $garage_data = _default_garage_data();

# Tesla API external process
my $tesla_event = Async::Event::Interval->new(0, \&update);
$tesla_event->start;

# Tesla API data timeout marker
my $last_conn_time = time;

hook before => sub {
    config_load();

    if (! security()) {
        my $struct = _default_data();
        $struct->{fetching} = 0;
        $struct->{error} = 1;
        halt(encode_json($struct));
    }
};

any ['get', 'post'] => '/' => sub {
    content_type 'application/json';

    return debug_data() if $tesla_conf->{debug_return};

    if (time - $last_conn_time > DATA_EXPIRY) {
        $tesla_data = '';
    }

    $last_conn_time = time;

    $tesla_event->start if $tesla_event->waiting;

    return $tesla_data if $tesla_data;
    return encode_json _default_data();
};
any ['get', 'post'] => '/debug' => sub {
    content_type 'application/json';
    return debug_data();
};
any ['get', 'post'] => '/debug_garage' => sub {
    content_type 'application/json';
    return debug_garage_data();
};
any ['get', 'post'] => '/wake' => sub {
    # Wake up the Tesla
    my $awoke_ok = eval {
        $car->wake;
        1;
    };

    if (! $car->online || ! $awoke_ok) {
        return "<html><body>Error code -1: Failed to wake the car</body></html>";
    }

    redirect '/';
};
any ['get', 'post'] => '/garage' => sub {
    # Main garage page (web)
    return template 'garage';
};
any ['get', 'post'] => '/garage_data' => sub {
    # Get garage data (microcontroller)
    content_type 'application/json';

    $garage_data->{relay_enabled} = $garage_conf->{relay_enabled};
    $garage_data->{app_enabled} = $garage_conf->{app_enabled};
    $garage_data->{auto_close_enabled} = $garage_conf->{auto_close_enabled};

    return encode_json $garage_data if $garage_data;
};
any ['get', 'post'] => '/garage_door_state' => sub {
    # Get garage door state
    return int $garage_data->{garage_door_state};
};
any ['get', 'post'] => '/garage_door_operate' => sub {
    # Trigger the garage door to operate (app/web)
    $garage_data->{activity} = 1;
};
post '/garage_update' => sub {
    # Update garage data (microcontroller JSON)

    my $data = decode_json request->body;

    $garage_data->{garage_door_state} = $data->{door_state};
    $garage_data->{activity} = $data->{activity};

    return;
};

dance;

sub security {
    my $secure = 1;

    if ($system_conf->{secure_host}) {
        my $allowed_hosts = $system_conf->{allowed_hosts};
        my @allowed_ips;

        for my $host (@$allowed_hosts) {
            if ($host =~ /[[:alpha:]]/) {
                # DNS A record
                my $packed_ip = gethostbyname($host);

                if (! defined $packed_ip) {
                    print "'$host' doesn't resolve to an IP address, skipping\n";
                    next;
                }

                my @ip_sections = map { hex($_) } map { /(..)/g } unpack("H*", $packed_ip);
                my $ip = join '.', @ip_sections;

                push @allowed_ips, "$ip/32";
                next;
            }
            if ($host !~ /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}(?:\/\d{1,2})?$/) {
                # Invalid IP
                print "'$host' is an incorrectly formatted IP address, skipping\n";
                next;
            }
            if ($host !~ /\/\d{1,2}$/) {
                # No prefix, add it
                $host = "$host/32";
                push @allowed_ips, $host;
                next;
            }

            push @allowed_ips, $host;
        }

        my $is_allowed_ips = subnet_matcher(@allowed_ips);
        my $requester_ip = request->address;

        if (! $is_allowed_ips->($requester_ip)) {
            print(
                sprintf "%s" , localtime->strftime('%F %T') .
                ": Failed to authenticate IP address '$requester_ip'\n"
            );
            $secure = 0;
        }
    }

    if ($system_conf->{secure_auth}) {
        my $token = '';

        if (body_parameters->get('token')) {
            $token = body_parameters->get('token');
        }
        elsif (from_json(request->body)) {
            $token = from_json(request->body)->{token};
        }

        my $saved_tokens = $system_conf->{tokens};

        if (! defined $token) {
            my $ip = request->address;
            print(
                sprintf "%s\n", localtime->strftime('%F %T') .
                    ": API token required but not sent in from IP $ip\n"
            );

            $secure = 0;
        }
        elsif (! grep { $token eq $_ } values %$saved_tokens) {
            my $ip = request->address;
                print(
                    sprintf "%s\n", localtime->strftime('%F %T') .
                    ": Failed to authenticate token: '$token' from IP $ip\n"
                );

            $secure = 0;
        }
    }

    return $secure;
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
    my $debug_data = $tesla_conf->{debug_data};

    my @data_items;

    for (sort {$a cmp $b} keys %$debug_data) {
        push @data_items, qq~"$_":$debug_data->{$_}~;
    }

    my $data_string = join ',', @data_items;

    print "$data_string\n";

    my $return_data = encode_json $debug_data;
    return $return_data;
}
sub debug_garage_data {
    return encode_json $garage_conf->{debug_data};
}
sub update {
    # If we're returning the debug data, there's no sense polling the
    # Tesla API

    return if $tesla_conf->{debug_return};

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
        $struct->{rainbow} = 1;
        return $struct;
    }

    $car->api_cache_clear;

    $online = $car->online;

    if (! $online) {
        $struct->{online} = 0;
        return $struct;
    }
    else {
        $chg        = $car->battery_level;
        $charging   = $car->charging_state;
        my $lat     = $car->latitude;
        my $lon     = $car->longitude;
        $gear       = $car->gear;

        $charging //= 'Disconnected';
        $charging = $charging eq 'Disconnected' ? 0 : 1;

        if (! defined $gear || $gear eq 'U') {
            print(
                sprintf "%s\n", localtime->strftime('%F %T') .
                ": Error: Corrupt JSON data from Tesla API.\n"
            );

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
        ? return abs(ACCURACY * ($latitude - $coord)) < 1
        : return abs(ACCURACY * ($longitude - $coord)) < 1;
}
sub distance {
    die "Need lat|lon and coord" if @_ != 2;

    my ($what, $coord) = @_;

    $what eq 'lat'
        ? return abs(ACCURACY * ($latitude - $coord))
        : return abs(ACCURACY * ($longitude - $coord));
}
sub gear {
    my ($gear) = @_;
    return 0 if $gear eq 'P';
    return 1 if $gear eq 'R';
    return 2 if $gear eq 'D';
    return 2 if $gear eq 'N';
}
sub _default_data {
    # Used in every API call to '/'

    my $struct = {
        online      => 0, # Vehicle is online
        garage      => 0, # Vehicle is in the garage
        charge      => 0, # Vehicle charge percent
        charging    => 0, # Vehicle is being charged
        gear        => 0, # Park: 0, Reverse: 1, Drive: 2
        error       => 0, # Tesla API error
        rainbow     => 0, # Rainbow LED mode
        fetching    => 1, # Currently fetching data from Tesla
        alarm       => $tesla_conf->{alarm}, # Alarm sound
    };

    return $struct;
}
sub _default_garage_data {
    # Used once at initial script run

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
