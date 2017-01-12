--TEST--
Intersection and XOR strict
--SKIPIF--
<?php if (!extension_loaded('typical')) echo 'skip';
--FILE--
<?php declare(strict_types=1);

class ArrayAccessableThing implements ArrayAccess {
  public function offsetGet($key) {}
  public function offsetSet($key, $val) {}
  public function offsetExists($key) {}
  public function offsetUnset($key) {}
}
$aat = new ArrayAccessableThing;

class CountableThing implements Countable {
  public function count() {}
}
$ct = new CountableThing;

class ArrayAccessableCountableThing implements ArrayAccess, Countable {
  public function offsetGet($key) {}
  public function offsetSet($key, $val) {}
  public function offsetExists($key) {}
  public function offsetUnset($key) {}
  public function count() {}
}
$aact = new ArrayAccessableCountableThing;

typical_set_type('AccessableCountable', 'ArrayAccess & Countable');
typical_set_type('AccessableOrCountable', 'ArrayAccess ^ Countable');

function aa(ArrayAccess $arg) {}
function c(Countable $arg) {}
function ac(AccessableCountable $arg) {}
function aoc(AccessableOrCountable $arg) {}

foreach (['aa', 'c', 'ac', 'aoc'] as $func) {
  foreach ([$aat, $ct, $aact] as $obj) {
    echo $func, '(', get_class($obj), ') = ';
    try {
      $func($obj);
      echo "OK\n";
    } catch (TypeError $e) {
      echo "TypeError\n";
    }
  }
}
--EXPECT--
aa(ArrayAccessableThing) = OK
aa(CountableThing) = TypeError
aa(ArrayAccessableCountableThing) = OK
c(ArrayAccessableThing) = TypeError
c(CountableThing) = OK
c(ArrayAccessableCountableThing) = OK
ac(ArrayAccessableThing) = TypeError
ac(CountableThing) = TypeError
ac(ArrayAccessableCountableThing) = OK
aoc(ArrayAccessableThing) = OK
aoc(CountableThing) = OK
aoc(ArrayAccessableCountableThing) = TypeError