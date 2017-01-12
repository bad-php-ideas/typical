--TEST--
Type aliasing parser error
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php declare(strict_types=1);

typical_set_type('Broken', 'Foo &| Bar');

--EXPECTF--

Fatal error: Uncaught TypeError: Syntax error at "|" (pos 6) in %s/join-002.php:3
Stack trace:
#0 %s/join-002.php(3): typical_set_type('Broken', 'Foo &| Bar')
#1 {main}
  thrown in %s/join-002.php on line 3
