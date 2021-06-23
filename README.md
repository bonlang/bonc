<div align="center">
	<h1>bonc</h1>
	<blockquote>
    👌 the compiler for the Bon programming language
	</blockquote>
  <p align="center">
		<a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg"></a>
	</p>
</div>

## About 

### Note: This project is extremely WIP, like no executables being produced WIP

Bon is a statically typed, functional, compiled programming language with (WIP) backends for x86 and RISC-V.

## Building

The Bon compiler uses ``meson`` for its build system, so the build tool ``ninja`` is required as well.

Clone the repository and enter it.
```
git clone https://github.com/bonlang/bonc
```

```
cd bonc
```

Initialize the build folder with Meson and enter it.
```
mkdir build
```

```
meson setup build
```

```
cd build
```

Build the project.
```
ninja
```

The executable will be placed in the directory you ran ``ninja`` and will be called ``bonc``.
## Usage

```
bon -h
``` 
will give you usage information.

## Language

``docs/language.md`` is your friend.
