# Bon Language

A bon program is made up of top level declarations.  The order of these is not critical as names can be used before their declaration.

## Basics

* A symbol is an ASCII string (no Unicode) starting with an upper or lowercase letter in the alphabet which can then be followed by letters, numbers or underscores.

## Types

### Builtin

* Int      - integer numbers (63 or 62 bits)
* Real     - IEEE 64 bit floating point number
* Bool     - either ``true`` or ```false```
* Char     - Unicode code point
* Byte     - 8 bits (or an ASCII char)

### User Defined

**not implemented**

## Toplevel declarations

A toplevel declaration has several parts
* A name (can also be _ if just the side effect is wanted)
* A list of names to be introduced into the functions scope
* A expression defining the value of the toplevel

### Examples (actual code omitted): 

```
myFun myVars = myExpr
```
```
_ = mySideEffect
```

## Expressions

The Bon language is made of expressions, even purely effectful computions are considered expressions, just with a Unit return type.

### Atomic expressions

* Integer literals  - ``123`` or ``-123``
* Boolean values - ``true`` or ``false``
* Real values - ``1.23``, ``1e12``, ``-0.12``
* Variable - Refers to the current value in the current lexical scope

### Compound

* Binary expressions - addition, subtraction, multiplication, division, equality, inequality, comparisons 
* Function calls  - Expressions seperated by spaces. Ex: (myFun 3 true)

### Full Examples

```
myConst = 3
addConst val = val + myConst

addTwoConst val1 val2 = (addConst val1) + (addConst val2)

_ = print_num (addTwoConst 10 15)
```
