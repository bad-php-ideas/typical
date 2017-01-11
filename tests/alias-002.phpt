--TEST--
Type aliasing loose
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php declare(strict_types=0);

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
           'Stringable', '?Stringable',
         ] as $type) {
  echo "** $type\n";

  $alias = '_'.crc32($type);
  typical_set_type($alias, $type);
  $func = create_function("$alias \$x", 'return $x;');
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
>> NULL -> bool(false)
>> 123 -> bool(true)
>> 123.4 -> bool(true)
>> "Hello" -> bool(true)
>> "234" -> bool(true)
>> array() -> TypeError
>> object -> TypeError

** ?bool
>> NULL -> NULL
>> 123 -> bool(true)
>> 123.4 -> bool(true)
>> "Hello" -> bool(true)
>> "234" -> bool(true)
>> array() -> TypeError
>> object -> TypeError

** int
>> NULL -> int(0)
>> 123 -> int(123)
>> 123.4 -> int(123)
>> "Hello" -> TypeError
>> "234" -> int(234)
>> array() -> TypeError
>> object -> TypeError

** ?int
>> NULL -> NULL
>> 123 -> int(123)
>> 123.4 -> int(123)
>> "Hello" -> TypeError
>> "234" -> int(234)
>> array() -> TypeError
>> object -> TypeError

** float
>> NULL -> float(0)
>> 123 -> float(123)
>> 123.4 -> float(123.4)
>> "Hello" -> TypeError
>> "234" -> float(234)
>> array() -> TypeError
>> object -> TypeError

** ?float
>> NULL -> NULL
>> 123 -> float(123)
>> 123.4 -> float(123.4)
>> "Hello" -> TypeError
>> "234" -> float(234)
>> array() -> TypeError
>> object -> TypeError

** string
>> NULL -> string(0) ""
>> 123 -> string(3) "123"
>> 123.4 -> string(5) "123.4"
>> "Hello" -> string(5) "Hello"
>> "234" -> string(3) "234"
>> array() -> TypeError
>> object -> string(10) "Stringable"

** ?string
>> NULL -> NULL
>> 123 -> string(3) "123"
>> 123.4 -> string(5) "123.4"
>> "Hello" -> string(5) "Hello"
>> "234" -> string(3) "234"
>> array() -> TypeError
>> object -> string(10) "Stringable"

** Stringable
>> NULL -> TypeError
>> 123 -> TypeError
>> 123.4 -> TypeError
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> object(Stringable)#1 (0) {
}

** ?Stringable
>> NULL -> NULL
>> 123 -> TypeError
>> 123.4 -> TypeError
>> "Hello" -> TypeError
>> "234" -> TypeError
>> array() -> TypeError
>> object -> object(Stringable)#1 (0) {
}
