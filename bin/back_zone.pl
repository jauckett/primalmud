#!/usr/bin/perl

use strict;

my $base = '/home/mud/newprimal/lib/world/';
my @subs = ('hnt','mob','obj','shp','trg','wld','zon');
my $targ = 'cf';

if ($ARGV[0] eq '') 
{
  print("Usage: $0 <output.tar> <zone> [zone] [zone] ...\n\n");
  exit 0;
}

my $outfile = $ARGV[0];

chdir($base);

if (-e $outfile)
{ 
  die("Remove $outfile first, please.\n"); 
}
for (my $i = 1; $i <= $#ARGV; $i++)
{
  for (my $j = 0; $j <= $#subs; $j++)
  {
    if (!(-e $subs[$j].'/'.$ARGV[$i].'.'.$subs[$j]))
    {
      print("NOT Added: $subs[$j]/$ARGV[$i].$subs[$j] (File not found.)\n");
      next;
    }
    print("    Added: $subs[$j]/$ARGV[$i].$subs[$j]\n");
    system("tar $targ $outfile $subs[$j]/$ARGV[$i].$subs[$j]");
    $targ = 'rf';
  }
}
