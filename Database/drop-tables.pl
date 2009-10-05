my @tables = ();
while (<>) { push @tables, $1 if /CREATE TABLE (\w+)/i; }
while (my $table = pop @tables) {
    print "IF EXISTS (SELECT * FROM INFORMATION_SCHEMA.TABLES ";
    print "WHERE TABLE_NAME = '$table') DROP TABLE $table\n";
}
