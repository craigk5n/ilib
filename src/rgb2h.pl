#!/usr/local/bin/perl

while ( <> ) {
  chop;
  if ( /^\s*(\d+)\s+(\d+)\s+(\d+)\s+(.*)$/ ) {
    $r = $1; $g = $2; $b = $3;
    $name = $4;
    $name = "\L$name";
    $name =~ s/ //g;
    #if ( $name !~ /\d/ ) {
      $r{$name} = $r;
      $g{$name} = $g;
      $b{$name} = $b;
    #} else {
    #  #print "Ignoring \"$name\"\n";
    #}
  } else {
    #print "Ignoring $_\n";
  }
}

@keys = keys ( %r );
$count = @keys;

print<<EOF;
typedef struct {
  char *name;
  unsigned char r, g, b
} INamedColorP;

#define I_NUM_NAMED_COLORS	$count

static INamedColorP[I_NUM_NAMED_COLORS] = {
EOF

foreach $val ( sort keys ( %r ) ) {
  printf "  { \"%s\", %d, %d, %d },\n", $val, $r{$val}, $g{$val}, $b{$val};
}

print<<EOF;
  { NULL, 0, 0, 0 }
};
EOF

exit 0;
