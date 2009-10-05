while (<>) {
	$xml = $_;
	open XML, ">CdrLogoff.xml" or die "Can't open xml file: $!";
	print XML $xml;
	close XML;
	system("CdrTestClient", "CdrLogoff.xml", "mmdb2");
}
