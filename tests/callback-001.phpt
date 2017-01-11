--TEST--
Callback typehint - strict
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php declare(strict_types=1);

typical_set_callback('num', function($arg, bool $strict) {
  if (is_int($arg) || is_float($arg) || (!$strict && is_numeric($arg))) {
    return $arg;
  }
  throw new TypeError('Bad Type!');
});

function f(num $x) {
  echo "foo($x)\n";
}

foreach ([123, 456.789, "234", array()] as $value ) {
  try {
    f($value);
  } catch (TypeError $e) {
    echo "Caught TypeError\n";
  }
}
--EXPECT--
foo(123)
foo(456.789)
Caught TypeError
Caught TypeError
