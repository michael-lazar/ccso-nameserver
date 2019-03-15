#!/usr/bin/perl

###
# This program is copyright Alec Muffett 1993. The author disclaims all
# responsibility or liability with respect to it's usage or its effect
# upon hardware or computer systems, and maintains copyright as set out
# in the "LICENCE" document which accompanies distributions of Crack v4.0
# and upwards.
###

sub ByLen {
	length($a) <=> length($b);
}

@a1 = ('/$s$s', '/0s0o', '/1s1i', '/2s2a', '/3s3e', '/4s4h');
@a2 = ('/$s$s', '/0s0o', '/1s1l', '/2s2a', '/3s3e', '/4s4h');
@a3 = ('/$s$s', '/0s0o', '/1s1i', '/2s2a', '/3s3e', '/4s4a');
@a4 = ('/$s$s', '/0s0o', '/1s1l', '/2s2a', '/3s3e', '/4s4a');

sub Permute {
	local(@args) = @_;
	local($prefix);

	while ($#args >= 0)
	{
		@foo = @args;
		$prefix = "";

		while ($#foo >= 0)
		{
			foreach (@foo)
			{
				$foo{"$prefix$_"}++;
			}
			$prefix .= shift(@foo);
		}

		shift(@args);
	}
}

&Permute(@a1);
&Permute(@a2);
&Permute(@a3);
&Permute(@a4);

foreach $key (sort ByLen (keys(%foo)))
{
	print $key, "\n";
}
