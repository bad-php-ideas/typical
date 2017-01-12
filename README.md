# Bad Idea: Complex type hints for PHP7

Use the `typical_set_type(string $alias, string $type)` and `typical_set_callback(string $alias, Callable $callback[, int $flags = 0])` functions to create psuedo typehints for use in running PHP7 scripts.  These `$alias` types may then be used as Type Hints to function/method declarations.

## Example uses

```php
typical_set_type('NullableInt', '?int');

/* These two functions have effectively the same signature */
function foo(NullableInt $x) {}
function bar(?int $x) {}
```
Types can be combined using `|`(OR), `&`(AND), or `^`(XOR) combinations:
```php
typical_set_type('Scalar', 'bool | int | float');
typical_set_type('IndexableAndIterable', 'ArrayAccess & Iterable');
typical_set_type('IteratorOrExceptionButNotBoth', 'Iterable ^ Exception');
```

Or for even more complex types, use a callback:
```php
typical_set_callback('Numberish', function ($arg, bool $strict) {
  if (is_int($arg) || is_float($arg)) {
    return $arg;
  }
  if (!$strict) {
    // Applies when called with strict_types=0 only
    if (is_null($arg) || is_bool($arg)) return intval($arg);

    if (is_object($arg)) $arg = strval($arg);
    if (is_numeric($arg)) return $arg;
  }
  throw new TypeError("Numberish type requires something numeric-like");
});

function double(Numberish $x) {
  return 2 * $x;
}
double(123); // OK
double(234.56); // OK
double("345.67"); // OK
double(new class() { function __toString() { return "5"; }}); // OK
double(new class() { function __toString() { return "hippo"; }}); // Error
double("banana"); // Error
```

## Known Limitations

This extension makes the runtime do things it does't want to do, occaisionally the runtime fights back.

### Initial values

Because the compiler sees aliases as class types, it complains about providing any default other than `null`.  Thus the following will fail to compile:

```php
typical_set_type('Foo', 'int');

function bar(Foo $x = 42) {}
```

## Future Scope

Things I may add with time.

### Negative Types

Prohibit accepting certain types, mostly useful with union/intersect types.

### Generics

It should be trivially possible to provide support for expressions such as:

```php
typical_set_type('IntArray', 'array<int>');
typical_set_type('Stream', 'resource<stream>');
```
