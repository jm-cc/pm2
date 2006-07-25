#!/usr/bin/perl -w
##################

use strict;
use Getopt::Std;

# conditional readline use
my $rl_mod = "Term::ReadLine";
my $term;

if (eval "require $rl_mod") {
    $term = new Term::ReadLine 'NewMad config';    
} else {
    print "warning: Perl/Readline lib not found, using regular prompt\n";
}

# sets
my %vars;
my %groups;
my @master_group_list;

sub cmdline ($);
sub cmdline ($) {
        s/\s+//g;

        my ($var_name, $op, $user_value) = /^([^\=]+)(=(.+)?)?$/;

        unless (defined $var_name) {
            print "syntax error\n";
            return;
        }

        unless (exists $vars{$var_name}) {
            print "unknown variable ${var_name}\n;";
            return;
        }

        if (defined $user_value) {
            ${$vars{$var_name}}{'value'}	= $user_value;
            ${$vars{$var_name}}{'user_value'}	= $user_value;
        } elsif (defined $op) {
            ${$vars{$var_name}}{'value'}	= '';
            ${$vars{$var_name}}{'user_value'}	= '';
        } else {
            print "$var_name: ${$vars{$var_name}}{'value'}\n";
        }
}

sub disp ($);
sub disp ($) {
    my $group_name	= shift;
    my $group	= $groups{$group_name};
    print "\n"
        unless ${$group}{'depth'};
    my $indent	= '..'x${$group}{'depth'};

    if (${$group}{'active'}) {
        print "${indent}[${$group}{'name'}: active]\n";
    } else {
        print "${indent}[${$group}{'name'}: inactive]\n";
        return;
    }

    $indent .= '..';
    for my $var (@{${$group}{'vars'}}) {
        print "$indent${$var}{'name'}:\t ${$var}{'value'}\n";

        for my $_group (keys %{${$var}{'subgroups'}}) {
            disp ($_group);
        }
    }
}

sub gen ($$);
sub gen ($$) {
    my $build_cfg_desc	= shift;
    my $group_name	= shift;
    my $group	= $groups{$group_name};

    return
        unless ${$group}{'active'};

    for my $var (@{${$group}{'vars'}}) {
        print $build_cfg_desc "${$var}{'name'}=${$var}{'value'}\n";

        for my $_group (keys %{${$var}{'subgroups'}}) {
            gen $build_cfg_desc, $_group;
        }
    }
}

sub gen_script($) {
    my $build_cfg_filename = shift;
    open my $build_cfg_desc, "> $build_cfg_filename"
        or die "could not open ${build_cfg_filename} for writing: $!\n";
    my $group_name;
    for $group_name (@master_group_list) {
        gen $build_cfg_desc, $group_name;
    }
}

sub process ($);
sub process ($) {
    my $group_name	= shift;
    my $group	= $groups{$group_name};

    return
        unless ${$group}{'active'};

    for my $var (@{${$group}{'vars'}}) {
        for my $_group_name (keys %{${$var}{'subgroups'}}) {
            my $_group	= $groups{$_group_name};

            if (${$var}{'type'} eq 'bool') {
                if (${$var}{'value'} eq 'y') {
                    ${$_group}{'active'} = 1;
                } else {
                    ${$_group}{'active'} = 0;
                }
            } else {
                die "${$var}{'name'} is not a boolean\n";
            }

            process (${$_group}{'name'})
                if ${$var}{'value'};
        }
    }

  set:
    for my $var (@{${$group}{'sets'}}) {
        print "set var ${$var}{'name'}\n";
        next set
            if exists ${$var}{'user_value'};

        ${$var}{'value'}	= ${${$group}{'defaults'}}{${$var}{'name'}};

        for my $_group_name (keys %{${$var}{'subgroups'}}) {
            my $_group	= $groups{$_group_name};

            if (${$var}{'type'} eq 'bool') {
                if (${$var}{'value'} eq 'yes') {
                    ${$_group}{'active'} = 1;
                } else {
                    ${$_group}{'active'} = 0;
                }
            } else {
                die "${$var}{'name'} is not a boolean\n";
            }

            process (${$_group}{'name'})
                if ${$var}{'value'};
        }
    }
}

sub update {
    my $group_name;

    for $group_name (@master_group_list) {
        process $group_name;
    }

    for $group_name (@master_group_list) {
        disp $group_name;
    }

    if (defined $term) {
        print "\n";
    } else {
        print "\n> ";
    }
}

# command line processing
my %opts;
$opts{'o'} = 'options.def';

getopts('o:', \%opts);
my $opt_filename = $opts{'o'};

print "using option file: ${opt_filename}\n";

my $build_cfg_filename;
if (@ARGV) {
    $build_cfg_filename	= shift @ARGV;
} else {
    $build_cfg_filename	= 'build.cfg';
}

print "using build cfg file: ${build_cfg_filename}\n";

# option definition file
open my $opt_desc, $opt_filename or die "could not open ${opt_filename}: $!\n";

# current group and dependency
my $group;
my $group_name;
my $dep_name;
my $dep;

line:
while (<$opt_desc>) {
    chomp();

    # skip empty lines
    next line if /^\s*$/;

    # skip comments
    next line if /^\s*#.*$/;

    # cleanup the line
    s/^\s+//;
    s/\s+$//;
    s/\s+/ /g;

    # are we reading a group header or not
    if (my ($gh) = /^\[(.+)\]$/) {
        # group header case

        # cleanup the line
        $gh =~ s/\s+//g;

        # extract group and possibly dependency name
        ($group_name, $dep_name) = ($gh =~ /([^\:]+)(?::(.+))?/);

        # check for duplicates
        die "duplicate group definition: $group_name\n"
            if exists ($groups{$group_name});

        # create group
        $group = {};
        ${$group}{'name'}	= $group_name;	# name of the group
        ${$group}{'vars'}	= [];		# var list
        ${$group}{'sets'}	= [];		# var list
        ${$group}{'defaults'}	= {};		# default values for variables

        if (defined $dep_name) {
            # sub group case
            if (exists $vars{$dep_name}) {
                $dep	= $vars{$dep_name};
                die "$dep_name is not a boolean var\n"
                    unless ${$dep}{'type'} eq 'bool';
            } else {
                die "undefined dependency var ${dep}\n";
            }

            ${$group}{'dep'}	= $dep_name;	# dependency
            ${$group}{'depth'}	= 1+${${$dep}{'group'}}{'depth'};	# depth
            ${${$dep}{'subgroups'}}{$group_name} = $group;
            ${$group}{'active'}	= 0;
        } else {
            # master group case
            $dep = undef;
            ${$group}{'depth'}	= 0;	# depth is 0 for master groups
            push @master_group_list, $group_name;
            ${$group}{'active'}	= 1;
        }

        # register group
        $groups{$group_name} = $group;
    } else {
        # simple entry

        # extract type/operator ('set')
        my ($type) = /^(\S+)\s/;
        die "invalid type ${type}\n" unless $type;

        # cleanup
        s/^(\S+)\s+//;
        s/\s+//g;

        # extract name and value
        my ($var_name, $value) = /^([^\=]+)(?:=(.+))?$/;

        die "syntax error: $_\n"
            unless (defined $var_name);

        $value = ''
            unless (defined $value);

        # create/update var
        my $var;
        if (exists $vars{$var_name}) {
            $var	= $vars{$var_name};
        } else {
            $var		= {};
            ${$var}{'name'}	= $var_name;
            $vars{$var_name}	= $var;
        }

        if ($type eq 'set') {
            die "illegal set in unconditionnal group"
                unless defined $dep;

            push @{${$group}{'sets'}}, $var;
        } else {
            die "duplicate definition of $var" if exists  ${$var}{'group'};
            ${$var}{'group'}	= $group;

            if (defined ${$var}{'type'}) {
                die "$var_name type mismatch: ${$var}{'type'} != $type\n"
                    unless ${$var}{'type'} eq $type;
            }

            ${$var}{'type'}	= $type;
            push @{${$group}{'vars'}}, $var;
        }

        ${${$group}{'defaults'}}{$var_name}	= $value;
        if ($type ne 'set') {
            ${$var}{'value'}	= $value;
        }
    }
}

if (-r $build_cfg_filename) {
    print "loading ${build_cfg_filename}\n";
    open my $build_cfg_desc, "$build_cfg_filename"
            or die "could not open ${build_cfg_filename} for reading: $!\n";

  load_line:
    while (<$build_cfg_desc>) {
        chomp;
        my $save_line;
        s/\s+//g;

        my ($var_name, $user_value) = /^([^\=]+)(?:=(.+)?)$/;

        die "syntax error: ${save_line}\n"
            unless defined $var_name;

        $user_value	 = ''
            unless defined $user_value;

        unless (exists $vars{$var_name}) {
            print "WARNING: ${var_name} does not exist\n";
            next load_line;
        }

        my $var	= $vars{$var_name};
        ${$vars{$var_name}}{'value'}		= $user_value;
        ${$vars{$var_name}}{'user_value'}	= $user_value;
    }

    close $build_cfg_desc
        or die "could not close ${build_cfg_filename}: $!\n";
}

update;

if (defined $term) {
    while ( defined ($_ = $term->readline('nm config> ')) ) {
        cmdline $_;
    } continue {
        print "\n";
        update;
    }
} else {
    while (<>) {
        chomp;
        cmdline $_;
    } continue {
        print "\n";
        update;
    }
}
gen_script($build_cfg_filename);
print "configuration file saved in: ${build_cfg_filename}\n";
