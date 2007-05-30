#!/usr/bin/perl -n
##################

use warnings;
use strict;
use Math::BigFloat;
use Getopt::Std;

BEGIN {
    # link rate and delay values are arbitrary
    our $link_rate	= 10000;
    our $link_delay	= 0.002;
    our $time_divisor	= Math::BigFloat->new('100000000');

    my %opts;
    my $ret	= getopts('d:r:t:', \%opts);	# -d <DELAY> -r <RATE> -t <TIME_DIVISOR>

    if (exists $opts{'d'}) {
        $link_delay	= $opts{'d'};
    }

    if (exists $opts{'r'}) {
        $link_rate	= $opts{'r'};
    }

    if (exists $opts{'t'}) {
        $time_divisor	= Math::BigFloat->new($opts{'t'});
    }

    our $FUT_NMAD_CODE	= 0xfe00;

    our $FUT_NMAD_EVENT0_CODE	= $FUT_NMAD_CODE + 0x00;
    our $FUT_NMAD_EVENT1_CODE	= $FUT_NMAD_CODE + 0x01;
    our $FUT_NMAD_EVENT2_CODE	= $FUT_NMAD_CODE + 0x02;
    our $FUT_NMAD_EVENTSTR_CODE	= $FUT_NMAD_CODE + 0x03;

    our $FUT_NMAD_EVENT_CONFIG_CODE		= $FUT_NMAD_CODE + 0x10;
    our $FUT_NMAD_EVENT_DRV_ID_CODE		= $FUT_NMAD_CODE + 0x11;
    our $FUT_NMAD_EVENT_DRV_NAME_CODE		= $FUT_NMAD_CODE + 0x12;
    our $FUT_NMAD_EVENT_CNX_CONNECT_CODE	= $FUT_NMAD_CODE + 0x13;
    our $FUT_NMAD_EVENT_CNX_ACCEPT_CODE		= $FUT_NMAD_CODE + 0x14;
    our $FUT_NMAD_EVENT_NEW_TRK_CODE		= $FUT_NMAD_CODE + 0x15;
    our $FUT_NMAD_EVENT_SND_START_CODE		= $FUT_NMAD_CODE + 0x16;
    our $FUT_NMAD_EVENT_RCV_END_CODE		= $FUT_NMAD_CODE + 0x17;

    our %fut_code_mapping;

    $fut_code_mapping{$FUT_NMAD_CODE + 0x00}	= 'EVENT0';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x01}	= 'EVENT1';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x02}	= 'EVENT2';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x03}	= 'EVENTSTR';

    $fut_code_mapping{$FUT_NMAD_CODE + 0x10}	= 'CONFIG';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x11}	= 'DRV_ID';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x12}	= 'DRV_NAME';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x13}	= 'CNX_CONNECT';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x14}	= 'CNX_ACCEPT';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x15}	= 'NEW_TRK';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x16}	= 'SND_START';
    $fut_code_mapping{$FUT_NMAD_CODE + 0x17}	= 'RCV_END';

    our %hosts;

#    our $config_size;
#    our $config_rank;
#    our %drivers;
#    our $drv_id;
#    our %gate2rank_mapping;
#    our %rank2gate_mapping;
    print "V -t * -v 1.0a5 -a 0\n";
}

our $link_rate;
our $link_delay;
our $time_divisor;

our $FUT_NMAD_CODE;

our $FUT_NMAD_EVENT0_CODE;
our $FUT_NMAD_EVENT1_CODE;
our $FUT_NMAD_EVENT2_CODE;
our $FUT_NMAD_EVENTSTR_CODE;

our $FUT_NMAD_EVENT_CONFIG_CODE;
our $FUT_NMAD_EVENT_DRV_ID_CODE;
our $FUT_NMAD_EVENT_DRV_NAME_CODE;
our $FUT_NMAD_EVENT_CNX_CONNECT_CODE;
our $FUT_NMAD_EVENT_CNX_ACCEPT_CODE;
our $FUT_NMAD_EVENT_NEW_TRK_CODE;
our $FUT_NMAD_EVENT_SND_START_CODE;
our $FUT_NMAD_EVENT_RCV_END_CODE;

our %fut_code_mapping;

our %hosts;

chomp;

my @params	= split ',';
my $num		= shift @params;
my $host_num	= shift @params;
my $ev_time	= Math::BigFloat->new(shift @params);
my $ev_tid	= shift @params;
my $ev_num	= shift @params;
my $ev_nb_params	= shift @params;

next
    unless (($ev_num & 0xff00) == 0xfe00);

my $time	= $ev_time->copy()->bdiv($time_divisor);

#printf "$num: \t$host_num - [%16u] %x = %16s \t", $ev_time, $ev_num, $fut_code_mapping{$ev_num};

if ($ev_num == $FUT_NMAD_EVENT_CONFIG_CODE) {
    my $config_size	= shift @params;
    my $config_rank	= shift @params;

    my $host	= {};
    ${$host}{'config_rank'}	= $config_rank;
    ${$host}{'config_size'}	= $config_size;
    ${$host}{'drivers'}		= {};
    ${$host}{'gate2rank'}	= {};
    ${$host}{'rank2gate'}	= {};
    $hosts{$host_num}	= $host;

    #print "$config_rank/$config_size";
    print "n -t * -a ${config_rank} -s ${config_rank} -v circle -c tan -i tan\n";
} elsif ($ev_num == $FUT_NMAD_EVENT_DRV_ID_CODE) {
    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};

    my $drv_id	= shift @params;
    ${$host}{'drv_id'}		= $drv_id;

    ${$drivers}{$drv_id}	= {};

    #print "id = $drv_id";
} elsif ($ev_num == $FUT_NMAD_EVENT_DRV_NAME_CODE) {
    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};

    unless (exists ${$host}{'drv_id'}) {
        print "\n";
        die "missing drv_id event\n";
    }

    my $drv_id	= ${$host}{'drv_id'};
    my $driver	= ${$drivers}{$drv_id};

    my $drv_name	= '';
    foreach my $param (@params) {
        $drv_name	.= pack ("V", $param);
    }

    ${$driver}{'name'}		= $drv_name;
    ${$driver}{'links'}		= {};
    ${$driver}{'pending_cnx'}	= [];

    delete ${$host}{'drv_id'};

    #print "name = $drv_name";
} elsif ($ev_num == $FUT_NMAD_EVENT_NEW_TRK_CODE) {
    my $host	= $hosts{$host_num};
    my $drivers	= ${$host}{'drivers'};

    my $gate_id	= shift @params;
    my $drv_id	= shift @params;
    my $trk_id	= shift @params;

    my $driver	= ${$drivers}{$drv_id};

    my $links	= ${$driver}{'links'};
    ${$links}{"$gate_id:$trk_id"}	= 1;

    my $pending_cnx	= ${$driver}{'pending_cnx'};
    push @{$pending_cnx}, [$gate_id, $drv_id, $trk_id];

    #print "gate id = $gate_id, drv id = $drv_id (${$driver}{'name'}), trk id = $trk_id";
} elsif ($ev_num == $FUT_NMAD_EVENT_CNX_CONNECT_CODE) {
    my $host		= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $rank2gate	= ${$host}{'rank2gate'};
    my $config_rank	= ${$host}{'config_rank'};

    my $remote_rank	= shift @params;
    my $gate_id		= shift @params;
    my $drv_id		= shift @params;
    my $driver	= ${$drivers}{$drv_id};

    unless (exists ${$gate2rank}{$gate_id}) {
        ${$gate2rank}{$gate_id}		= $remote_rank;
        ${$rank2gate}{$remote_rank}	= $gate_id;
    }

    #print "$config_rank -> $remote_rank, gate id = $gate_id, drv id = $drv_id (${$driver}{'name'})";
    print "l -t * -s ${config_rank} -d ${remote_rank} -S UP -c black -r ${link_rate} -D ${link_delay}\n";
    #print "l -t * -s ${config_rank} -d ${remote_rank} -S UP -c black\n";
} elsif ($ev_num == $FUT_NMAD_EVENT_CNX_ACCEPT_CODE) {
    my $host		= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $rank2gate	= ${$host}{'rank2gate'};
    my $config_rank	= ${$host}{'config_rank'};

    my $remote_rank	= shift @params;
    my $gate_id		= shift @params;
    my $drv_id		= shift @params;
    my $driver	= ${$drivers}{$drv_id};

    unless (exists ${$gate2rank}{$gate_id}) {
        ${$gate2rank}{$gate_id}		= $remote_rank;
        ${$rank2gate}{$remote_rank}	= $gate_id;
    }

    #print "$config_rank <- $remote_rank, gate id = $gate_id, drv id = $drv_id (${$driver}{'name'})";
} elsif ($ev_num == $FUT_NMAD_EVENT_SND_START_CODE) {
    my $host		= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $config_rank	= ${$host}{'config_rank'};

    my $gate_id	= shift @params;
    my $drv_id	= shift @params;
    my $trk_id	= shift @params;
    my $length	= shift @params;

    my $remote_rank	= ${$gate2rank}{$gate_id};
    my $driver		= ${$drivers}{$drv_id};
    my $drv_name	= ${$driver}{'name'};

    #print "$config_rank -> $remote_rank, drv id = $drv_id ($drv_name), trk id = $trk_id, length = $length";

    print "+ -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
    print "- -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
    print "h -t ${time} -e ${length} -s ${config_rank} -d ${remote_rank}\n";
} elsif ($ev_num == $FUT_NMAD_EVENT_RCV_END_CODE) {
    my $host	= $hosts{$host_num};

    my $drivers		= ${$host}{'drivers'};
    my $gate2rank	= ${$host}{'gate2rank'};
    my $config_rank	= ${$host}{'config_rank'};

    my $gate_id	= shift @params;
    my $drv_id	= shift @params;
    my $trk_id	= shift @params;
    my $length	= shift @params;

    my $remote_rank	= ${$gate2rank}{$gate_id};
    my $driver		= ${$drivers}{$drv_id};
    my $drv_name	= ${$driver}{'name'};

    #print "$config_rank <- $remote_rank, drv id = $drv_id ($drv_name), trk id = $trk_id, length = $length";

    print "r -t ${time} -e ${length} -s ${remote_rank} -d ${config_rank}\n";
}
#print "\n";
