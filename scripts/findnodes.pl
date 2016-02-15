#!/usr/bin/env perl
use strict;
use warnings;

##
# added __linux check since well, this only works
# for systems with /proc fs. Need to check Win
# for NUMA control.
##
if( defined $ENV{ "__linux" } )
{
    my $cmdstr =  "grep -oP \"(?<=Node\\s)\\d\" /proc/zoneinfo";
    my $nodelist = `$cmdstr`;
    chomp( $nodelist );
    my @arr = split/\n/,$nodelist;
    my $first = shift( @arr );
    foreach my $val ( @arr )
    {
        if( $val != $first )
        {
            print( 1 );
            exit( 0 );
        }
    }
}
print( 0 );
exit( 0 );
