# Luciforth

A satanic Forth in plain C.

Luciforth is a small, indirect threaded Forth system. The virtual machine is
called **the Abattoir**, the dictionary is **the Grimoire**, and the prompt is
`666>`.

## Build

```sh
make
```

## Run

```sh
./luciforth
```

One newline is needed to display the first prompt.

## Words

Numbers and `"`-delimited strings are literals; strings push the address of
the most recently read string.

| Word | Stack effect | Meaning |
|------|--------------|---------|
| `hail` | `( -- )` | print `HAIL LUCIFER` |
| `sacrifice` | `( x -- )` | discard the top of stack |
| `grimoire` | `( -- )` | list the dictionary |
| `.` | `( x -- )` | print and pop the top of stack |
| `cr` | `( -- )` | emit a newline |
| `type` | `( addr -- )` | print the string at `addr` |
| `+` | `( a b -- c )` | add the top two cells |
| `*` | `( a b -- c )` | multiply the top two cells |
| `dup` | `( x -- x x )` | duplicate the top of stack |
| `:` … `;` | | define a new word |
| `bye` | `( -- )` | leave the Abattoir |

## Example

```text
$ ./luciforth
Luciforth - the Abattoir is open.
666> 2 3 + .
5 666> hail
HAIL LUCIFER
666> : sq dup * ;
666> 5 sq .
25 666> "Hello World" type cr
Hello World
666> bye
The Abattoir falls silent.
```

While a definition is open the prompt reads `compile>`.

## How it works

- One data stack of untyped `long long` cells carries every value, and a
  separate return stack carries return addresses.
- A word is an execution token (`xt`): a name, the host function that
  implements it, and a pointer into the code space.
- Compiled words are indirect threaded code: the code space holds a plain
  array of execution-token pointers, and `vm()` fetches one token at a time
  and calls its primitive.
- `:` defines a word and enters compile mode; `;` is an immediate word that
  compiles a return and leaves compile mode. Compile-time words live in a
  separate macro dictionary.
- The outer interpreter itself is a threaded word (`shell`), compiled by hand
  at start-up.

## Provenance

Derived from the public-domain FORTH VM sources by Andreas Klimas
(klimas@w3group.de).

## License

MIT. See [LICENSE](LICENSE).
