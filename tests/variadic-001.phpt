--TEST--
Variadic Typehints
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php declare(strict_types=0);

typical_set_callback('num', function($arg, bool $strict) {
  if (is_int($arg) || is_float($arg) || (!$strict && is_numeric($arg))) {
    return $arg;
  }
  throw new TypeError('Bad Type!');
});

function f(num ...$xs) {
  foreach ($xs as $i => $x) {
    echo "$i: foo($x)\n";
  }
}

$args = [
  [ 1, 2, 3, 4 ],
  [ 1.1, 2.2, 3.3, 4.4 ],
  [ 1, 2.3, "4" ],
  [ 5, 6, "banana" ],
];

foreach ($args as $values ) {
  try {
    f(...$values);
  } catch (TypeError $e) {
    echo "Caught TypeError\n";
  }
}
--EXPECT--
0: foo(1)
1: foo(2)
2: foo(3)
3: foo(4)
0: foo(1.1)
1: foo(2.2)
2: foo(3.3)
3: foo(4.4)
0: foo(1)
1: foo(2.3)
2: foo(4)
Caught TypeError