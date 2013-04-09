#!/usr/bin/perl -w

use strict;
use warnings;

=head1 NAME

ndt_log_delete.pl

=head1 DESCRIPTION

Given a 'safe date' from an external application, delete
the log directories for an NDT server.

=cut

my $hostname = shift or die "Please provide hostname.\n";

open( READ, "/home/iupui_ndt/./get_safe_delete_date.py rsync://ndt.iupui." . $hostname . ":7999/ndt-data 
|" );
my @date = <READ>;
close( READ );

die "Date not found from 'get_safe_delete_date.py', aborting.\n" unless $date[0];
$date[0] =~ s/-//g; 

my $dir = "/usr/local/ndt/serverdata";

dirwalk($dir);

exit(1);

sub dirwalk {
  my $dirname = shift;

  opendir ( DIR, $dirname ) or die "Error in opening dir $dirname\n";
  my @dir = readdir(DIR);
  closedir(DIR);
  foreach my $filename ( sort @dir ) {
     next if $filename eq ".." or $filename eq ".";
     next unless -d $dirname . "/" . $filename;

     dirwalk( $dirname . "/" . $filename );
     my $comp = substr( $dirname."/".$filename, length( $dir ) );
     $comp =~ s/\///g;
     if ( length $comp == 8 and $comp < $date[0] ) {
#        print "rm -frd " . $dirname . "/" . $filename . "\n";
        system( "rm -frd " . $dirname . "/" . $filename );
     }
  }
  return;
}

__END__

=head1 VERSION

$Id$

=head1 AUTHOR

Jason Zurawski, zurawski@internet2.edu

=head1 LICENSE

You should have received a copy of the Internet2 Intellectual Property Framework
along with this software.  If not, see
<http://www.internet2.edu/membership/ip.html>

=head1 COPYRIGHT

Copyright (c) 2010, Internet2

All rights reserved.

=cut

