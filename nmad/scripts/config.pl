#!/usr/bin/perl -w
##################

use strict;
use Getopt::Std;

## Global variables

# conditional readline use
my $rl_mod = "Term::ReadLine";	# Readline generic module name
my $use_rl;			# defined if RL is in use
my $rl;				# actual RL implementation
my $term;			# RL terminal object

# sets
my %vars;			# variables
my %groups;			# groups of variables
my @master_group_list;		# list of top level groups

# RL command line completion data
my @completion_matches;		# list of current cmpl matches
my $completion_i;		# current cmpl match
my $completion_var_name;	# current cmpl var name
my $completion_var;		# current cmpl var

## Routines

sub varname_completion_f (){
    my($text, $state) = @_;

    # print "looking for a var starting with [$text]\n";

    if ($state) {
        $completion_i++;
    } else {
        $completion_i = 0;
        @completion_matches = sort keys %vars;
    }

    for (; $completion_i <= $#completion_matches; $completion_i++) {
        if ($completion_matches[$completion_i] =~ /^\Q$text/i) {
            return $completion_matches[$completion_i];
        }
    }

    return undef;
}

sub var_completion_f (){
    my($text, $state) = @_;

    # print "looking for a var starting with [$text]\n";

    if ($state) {
        $completion_i++;
    } else {
        $completion_i = 0;

        if (${$completion_var}{'type'} eq 'bool') {
            @completion_matches = ('y', 'n');
        } else {
            return undef;
        }
    }

    for (; $completion_i <= $#completion_matches; $completion_i++) {
        if ($completion_matches[$completion_i] =~ /^\Q$text/) {
            return $completion_matches[$completion_i];
        }
    }

    return undef;
}

sub attempt_completion_f ($$$$);
sub attempt_completion_f ($$$$) {
    my ($text, $line, $start, $end) = @_;
    my $begin = substr($line, 0, $start);

    if ($begin =~ /^\s*$/ and $text =~ /^(?:\w|_)*$/) {
        return $term->completion_matches($text,
					 \&varname_completion_f);
    }

    if ($begin =~ /^\s*(?:\w|_)[^=]+=\s*$/) {
        ($completion_var_name) = ($begin =~ /^\s*(\S+)\s*=/);

        return undef
            unless defined $completion_var_name;

        return undef
            unless exists $vars{$completion_var_name};

        $completion_var = $vars{$completion_var_name};

        return $term->completion_matches($text,
					 \&var_completion_f);
    }

    return undef;
}

sub cmdline ($);
sub cmdline ($) {
    s/\s+//g;

    return 0
        if /^$/;

    my ($var_name, $op, $user_value) = /^([^\=]+)(=(.+)?)?$/;

    unless (defined $var_name) {
        print "syntax error\n";
        return 0;
    }

    unless (exists $vars{$var_name}) {
        print "unknown variable ${var_name}\n";
        return 0;
    }

    my $var = $vars{$var_name};

    if (${$var}{'type'} eq 'bool') {
        $user_value	= lc $user_value;
        unless ($user_value =~ /[yn]/) {
            print "invalid argument\n";
            return 0;
        }
    }

    if (defined $user_value) {
        ${$var}{'value'}	= $user_value;
        ${$var}{'user_value'}	= $user_value;

        return 1;
    } elsif (defined $op) {
        ${$var}{'value'}	= '';
        ${$var}{'user_value'}	= '';

        return 1;
    }

    print "$var_name: ${$vars{$var_name}}{'value'}\n";
    return 0;
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

    if (defined $use_rl) {
        print "\n";
    } else {
        print "\n> ";
    }
}

## Main program
my %opts;
my $opt_filename;
my $opt_desc;
my $build_cfg_filename;

# current group and dependency
my $group;
my $group_name;
my $dep_name;
my $dep;

print "NewMadeleine configurator\n";
print "-------------------------\n";

# command line processor initialization
if (eval "require $rl_mod") {
    # use RL if available

    $term	= new Term::ReadLine 'NewMad config';
    $term->Attribs->{attempted_completion_function}	= \&attempt_completion_f;
    $term->Attribs->{completion_append_character}	= '';

    if (exists $term->Features->{'ornaments'}) {
        $term->ornaments('md,me,,');
    }
    $use_rl	= 1;
    $rl		= $term->ReadLine;

    print "using ${rl} module for input management\n";
} else {
    print "warning: no Perl/Readline lib found, using regular prompt\n";
}

# command line arguments
$opts{'o'}	= 'options.def';
getopts('o:', \%opts);
$opt_filename	= $opts{'o'};

if (@ARGV) {
    $build_cfg_filename	= shift @ARGV;
} else {
    $build_cfg_filename	= 'build.cfg';
}

print "using option file: ${opt_filename}\n";
print "using build cfg file: ${build_cfg_filename}\n";

# load option definition file
open $opt_desc, $opt_filename or die "could not open ${opt_filename}: $!\n";

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

# load configuration file if exists
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

# update variable state
update;

# command line processing
if (defined $use_rl) {
    while ( defined ($_ = $term->readline('nm config> ')) ) {
        if (cmdline $_) {
            update;
        }
    }
    print "\n";
} else {
    while (<>) {
        chomp;
        if (cmdline $_) {
            update;
        }
    }
}
gen_script($build_cfg_filename);
print "configuration file saved in: ${build_cfg_filename}\n";
