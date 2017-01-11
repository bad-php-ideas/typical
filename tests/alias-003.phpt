--TEST--
Type aliasing strict w/ defaults
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php declare(strict_types=1);

class Stringable {
  public function __toString() {
    return "Stringable";
  }
}
$stringable = new Stringable;

foreach ([ 'bool',       '?bool',
           'int',        '?int',
           'float',      '?float',
           'string',     '?string',
         ] as $type) {
  echo "** $type\n";

  $alias = '_'.crc32($type);
  typical_set_type($alias, $type);
  $func = create_function("$alias \$x = null", 'return $x;');

  echo '>> (nothing) -> ';
  try {
    $ret = $func();
    var_dump($ret);
  } catch (TypeError $e) {
    echo "TypeError\n";
  }

  foreach ([null, 123, 123.4, "Hello", "234", array(), $stringable] as $value) {
    if ($value === null) {
      echo '>> NULL -> ';
	} elseif (is_string($value)) {
      echo ">> \"$value\" -> ";
    } elseif (is_array($value)) {
      echo '>> array() -> ';
    } elseif (is_object($value)) {
      echo '>> object -> ';
    } else {
      echo ">> $value -> ";
    }

    try {
      $ret = $func($value);
      var_dump($ret);
    }  catch (TypeError $e) {
      echo "TypeError\n";
    }
  }
  echo "\n";
}
--EXPECTF--
** bool
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> TypeError
>> 123.4 -> TypeError
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> TypeError

** ?bool
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> TypeError
>> 123.4 -> TypeError
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> TypeError

** int
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> int(123)
>> 123.4 -> TypeError
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> TypeError

** ?int
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> int(123)
>> 123.4 -> TypeError
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> TypeError

** float
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> float(123)
>> 123.4 -> float(123.4)
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> TypeError

** ?float
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> float(123)
>> 123.4 -> float(123.4)
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> TypeError

** string
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> TypeError
>> 123.4 -> TypeError
>> "Hello" -> string(5) "Hello"
>> "234" -> string(3) "234"
>> array() -> TypeError
>> object -> TypeError

** ?string
>> (nothing) -> NULL
>> NULL -> NULL
>> 123 -> TypeError
>> 123.4 -> TypeError
>> "Hello" -> string(5) "Hello"
>> "234" -> string(3) "234"
>> array() -> TypeError
>> object -> TypeError