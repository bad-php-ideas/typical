--TEST--
Type parsing
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php

typical_set_type('Foo', '?int');

var_dump(typical_get_type('Foo'));
--EXPECT--
array(4) {
  ["flags"]=>
  int(1)
  ["type"]=>
  int(0)
  ["type_hint"]=>
  int(4)
  ["class_name"]=>
  NULL
}
