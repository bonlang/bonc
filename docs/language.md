# Beans Language

A beans program is made up of a series of functions, the order of these functions does not matter and they can be referenced before being called.

## Basics

* A symbol is an ASCII string starting with a letter from A-Z and then followed by letters, numbers, or an underscore.

## Types

There are several builtin types. 
* void 		 - represents a lack of a value
* i8/i16/i32/i64 - signed integer types with the specified number of bits
* u8/u16/u32/u64 - unsigned integer types with the specified number of bits
* bool 		 - either ``true`` or ``false``

User defined types are not implemented, and the design is still changing.

## Functions

A function has several parts.
* A name - a symbol
* A list of parameters wrapped in parenthesis where a parameter is a symbol followed by a type
* An optional return type after the parameters, if omitted the return type is void
* A block of statements wrapped in curly braces.

### Examples (actual code omitted): 

```
add2(v1 i32, v2 i32) i32 {}
```

```
main() {}
```

## Statements

A statement makes up functions, but beans has most language features as expressions so they can be composed in powerful ways. However there are still a couple features that are only represented as statements.

* Let        - introduces a new name into the current local scope, optionally giving it an initial value
* Return     - returns from the current function, optionally with a value
* Expression - just an expression which is computed, and the result is discarded

Statements can either be terminated with a semicolon (;) or with a newline.  The general style is to always use a newline unless several statements are to be placed on the same line, which should very rarely be done.

### Examples (expressions are omitted)

```
let my_var = <expression>
<expression>
return my_var
```

## Expressions

Most of the beans language is represented as expressions, so there are going to be many possible types of expressions.

* Integers - A single integer literal optionally terminated with the name of one of the integer types to specify what size it is
* Booleans - ``true`` or ``false``
* Variable - The value of a variable in the local scope
* Binary expressions - addition, subtraction, multiplication, division, equality, inequality, comparisons 
* Function calls 

```
3
3u32
my_var
3 + my_var
3 != (my_var * 4)
add2(my_var, 4) != 10
```


### Full Examples

```
main() {
	let a = 3i32
	let sum = add2(a, 3 + 5)
	sum
}

add2(v1 i32, v2 i32) i32 {
	return v1 + v2
}
```
