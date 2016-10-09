# cl - combinatory logic interpreter

This document describes how to build and use `cl`. `cl` interprets
a programming language with a lot of similarities to various Combinatory Logic (CL)
formal systems. It doesn't exactly interpret any Combinatory Logic formal system in that
it runs on computers with finite CPU speed and a finite memory.
Most or all formal systems fail to take these limits into account.

I started writing this interpreter as a way to test a graph reduction
implementation of a [lambda calculator](https://github.com/bediger4000/lambda-calculator).
Along the way, I bought a copy of Raymond Smullyan's
[To Mock A Mockingbird](http://www.amazon.com/Mock-Mockingbird-Raymond-M-Smullyan/dp/0192801422/ref=sr_1_3?ie=UTF8&amp;s=books&amp;qid=1226791982&amp;sr=8-3) .
While working that book's problems,
I ended up making `cl` into a reasonably complete and usable system.

## Building and installing

Try one of these commands:

    make cc       # very generic, Solaris or *BSD
    make gnu      # all GNU, most/all linux distros
    make clang    # clang compiler

For most or all Linux distros with development tools installed, `make gnu` should work.
The last time I had access to Solaris, `make cc` worked. I believe it will work on *BSD
systems.  `make clang` is for those upright citizens with the Clang compiler set installed.

At this point, you can test the newly-compiled executable.
From the command line: `./runtests`  .  Most of the tests should
run quite rapidly, in under a second.  At least two of the tests run
for 30 seconds or so, and at least one of the tests provokes
a syntax error message from the interpreter.

Install the interpreter if you want, or you can execute it in-place.
To install, use the `cp` or `mv` commands to move or
copy the executable to where ever you want it.  It does not care what
directory it resides in, and it does not look for configuration files
anywhere.

## Using the interpreter

The interpreter uses `CL>` as its prompt for user input.
`cl` has a strict grammar, so you must type either
a term for reduction, or an interpreter command,
or a command to examine a term.

You have to use keyboard end-of-file (usually control-D) to exit `cl`

Giving the interpreter a CL term causes it to execute
a read-eval-print loop.
After reading and parsing the input,
the interpreter prints the input in a minimally-parenthesized representation,
reduces it to normal form, and prints a text representation of the normal form.
It prints a prompt, and waits for another user input.


`cl` does "normal order" evaluation: the leftmost outermost redex
gets evaluated first.  This seems like the standard for CL, unlike
for lambda calculus, where a lot of ink gets expended distinguishing between
"normal order"
and "applicative order".

Expressions consist of either a single term, or two (perhaps implicitly
parenthesized) terms.
Terms consist of either a
built-in primitive or a variable, or a
parenthesized expression.

Built-in pimitives have names consisting of a single upper-case letter.
Variables (which can also serve as abbreviations)
can look like C or Java style identifiers: a letter, followed by zero
or more letters or underscores.
You cannot define a variable (or an abbreviation) with the same name
as a built-in pimitive.

The interpreter treats primitives and variables as "left associative",
the standard in the Combinatory Logic literature.
That means that an expression like this:
`I a b c d` ends up getting treated as though it had parentheses
like this: `((((I a) b) c) d)`

To apply one complex term to another, the user must use parentheses.
Applying `W (W K)` to `C W` would look like this:
`(W (W K)) (C W)`.

####Parentheses

Users can parenthesize input expressions as much or as little as they desire,
up to the limits of left-association and the meaning they wish to convey
to the interpreter.
The grammar used by `cl` does not
allow single terms inside paired parentheses. It considers strings
like `(I)` as syntax errors. You have to put at least two terms
inside a pair of parentheses, and parentheses must have matches.

The interpreter prints out normal forms in minimal-parentheses style.
Users have the ability to cut-n-paste output back into the input,
as output has valid syntax.  No keyboard shortcuts exist to re-use 
any previous output.

###Built-in Primitives

I built-in 9 primitive combinators. They contract like this:

* _I_ a &rarr; a
* _K_ a b &rarr; a
* _S_ a b c &rarr; a c (b c)
* _B_ a b c &rarr; a (b c)
* _C_ a b c &rarr; a c b
* _W_ a b &rarr; a b b
* _T_ a b &rarr; b a
* _M_ a &rarr; a a
* _J_ a b c d &rarr; a b (a d c)

Where `a, b, c, d` represent any CL term.

This set of built-ins lets you use `{S, K}`, `{S, K, I}`,
`{B, W, C, K}`, `{B, M, T, K}` bases for &lambda;K calculi,
or
`{J, I}`, `{B, C, W, I}` and `{I, B, C, S}` as bases for &lambda;I calculi.


Built-in combinators require a certain number (one to four) of arguments
to cause a contraction.  They just sit there without that number of arguments.


You can "turn off" any of the nine combinators as built-ins with a `-C X`
command line option (`X` is any of the nine built-in combinators).
No interpreter command exists to turn off or on a combinator during a session.


###Bracket Abstraction

Bracket abstraction names the process of creating from an original CL expression, a
CL expression without specified variables, that when evaluated with appropriate arguments, ends up
giving you the original expression with argument(s) in the place of the specified variables.

The `cl` interpreter uses the conventional square-bracket
notation.  For example, to create an expression that will duplicate
its single argument, one would type:

> CL> [x] x x

You can use more than one variable inside square brackets, separated
with commas:

> CL> [a, b, c] a (b c)

The above square-bracketed expression ends up working through three
bracket abstraction operations, abstracting `c` from `a (b c)`,
`b` from the resulting expression, and `a` from
that expression. You can nest bracket abstractions: `[a][b][c] a (b c)`
should produce the same expression as the example above.

A bracket abstraction makes an expression, so you can use them where ever
you might use any other simple or complex expression, defining an abbreviation,
a sub-expression of a much larger expression, as an expression to evaluate
immediately, or inside another bracket abstraction.
For example, you could create Turing's fixed-point combinator like this:

    CL> def U [x][y] (x(y y x))
    CL> def Yturing (U U)

####Bracket Abstraction Algorithms

`cl` offers seven bracket abstraction algorithms:

* `curry` - classic, minimalistic three-rule system for {S,K,I} basis.
* `curry2` - four-rule system for {S,K,I} basis. Hindley and Seldin's Definition 2.18
* `tromp` - John Tromp's nine-rule system for compact results in the {S,K,I} basis.
* `turner` - Simplified Turner algorithm, using S,B,C,K,I combinators.
* `grz` - the Grzegorczyk algorithm for {B,W,C,K} basis.
* `btmk` - my homegrown algorithm for {B,T,M,K} basis.
* `church` - algorithm for {I,J} basis.


You can set a default algorithm with the this command: `abstraction name`. For
*name*, substitute one of the seven algorithm names above.  `cl` starts with
`curry` as the default bracket abstraction algorithm.

You can specify the abstraction algorithm next to the abstracted variable:

    CL> [x]btmk (x (K x))

###Defining abbreviations

* `define name expression`
* `def name expression`
* `reduce expression`

These usages allow a user to introduce abbreviations
to the input. Each time the abbreviation appears in input, `cl` makes a copy
of the expression so abbreviated, and puts that copy in the input.
No matter how complex the expression, an
abbreviation still comprises a single term. Effectively, the interpreter puts
the expanded abbreviation in a pair of parentheses.

`def` makes an easy-to-type abbreviation of `define`.

The `reduce` command produces an expression by performing normal order
contractions on the expression, out-of-order from the usual contractions.
Unlike `define` or `def`, you can use `reduce` anywhere an expression would
fit, as part of a larger expression, as part of an abbreviation, or as part of
a bracket abstraction.

###Information about expressions

* `size expression` - print the number of atoms in *expression*.
* `length expression` - print the number of atoms plus number of applications in *expression*.
* `print expression` - print human-readable representation of *expression*, with abbreviations expanded, but without evaluation.
* `redexes expression` - print a count of possible contractions in *expression*, regardless of order of evaluation.
* `expression1 = expression2` - determine lexical equivalence of any two expressions, after abbreviation substitution, but without evaluation.

`print` lets you see what abbreviations expand to, without evaluation.  The "="
sign lets you determine lexical equality.  All combinators, variables
and parentheses have to match as strings, otherwise "=" deems the expressions
not equivalent.  You can put in explicit `reduce` commands on both
sides of an "=", otherwise, no evaluation takes place.

`size` and `length` seem redundant, but authorities
measure CL expressions different ways.  These two methods should cover the
vast majority of cases.

###Intermediate output and single-stepping

* `trace on|off`
* `debug on|off`
* `step on|off`

You can issue any of these commands without an "on" or "off" argument
to inquire about the current state of the directive.

`trace` and `debug` make for
increasingly verbose output. `trace` shows the expression
after each contraction, `debug` adds information about which
branch of an application it evaluates.

`detect` causes `trace` to also print a count
of possible contractions (not all of them normal order reductions),
and mark contractable primitives with an asterisk.

`step on` causes the interpreter to stop after each contraction,
print the intermediate expression, and wait, at a `?` prompt
for user input. Hitting return goes to the next contraction, `n`
or `q` terminates the reduction, and `c` causes it
to quit single-stepping.

###Reduction information and control

* `timer on|off` - turn on/off per-reduction elapsed time output.
* `timeout 0|N`- stop reducing after *N* seconds.
* `count 0|N` - stop reducing after *N* contractions.
* `cycles on|off`
* `detect on|off`

You can turn time outs off by using a 0 (zero) second timeout.
Similarly, you can turn contraction-count-limited-evaluation off
with a 0 (zero) count.

`timer on` also times bracket abstraction.

Some CL expressions end up creating a cycle: `M M` or `W W W`
or `B (W K) (W (W K)) (B (W K) (W (W K)))`.  After a certain number of
contractions, the interpreter ends up with an expression it has already encountered.
If you issue the `cycles on` command, the interpreter keeps track of
every expression in the current reduction, and stops when it detects a cyclical
reduction.

detect on` causes the interpreter to count and mark primitives eligible
for contraction (with an asterisk),
regardless of reduction order.  It does "normal order" reduction, but ignores that
for the contraction count. This only has utility with `trace on`.

Turning cycle detection on will add time to an
expression's reduction, as will possible contraction detection.

####Matching a pattern during reduction

`cl` v1.6 adds a way to control reduction: stop when the (partially)
reduced expression matches a pattern.
`cl` can perform pattern matching after each contraction, when
the user sets a pattern using the `match` command.
`unmatch` destroys the internal state created by `match`,
so the user can't recall a previous `match` except by complete
re-entry of the command.

* `match pattern` - stop reduction if and when pattern</em> appears.
* `unmatch` - relieve the interpreter of checking for a pattern match during reduction.

Patterns look like expressions with one exception.
An asterisk ('*') acts as a wildcard, matching any expression.
Any other variable or built-in primitive ocurring in the pattern matches
itself literally.

For example, issuing the command `match S K K` would cause the reduction of
`S (I K) (S K) K` to stop after a single contraction.

###Reading in files

* `load "filename"`

You have to double-quote filenames with whitespace or
non-alphanumeric characters in them.
You can use absolute filenames (beginning with "/") or you can
filenames relative to the current working directory of the `cl`
process.

The `load` command works during a session.  You can get the same
effect at interpreter start-up with the `-L filename` command line flag.

##Examples

The `tests.in/` directory of this repository has a lot of files. Some are just
to ensure that old bugs don't show up again, some just test single features.
Other files are decent examples
* `tests.in/test.033`: Church numerals and Ackermann function
* `tests.in/test.043`: Scott numerals
* `tests.in/test.044`: Scott numerals with a "non-standard" fixed point combinator
* `tests.in/test.059`: Scott numerals with lots of unusual fixed point combinators
* `tests.in/test.063`: Ogre, eater or Egocentric Bird terms
* `tests.in/test.065`: An abuse of fixed point combinators
* `tests.in/test.076`: "Monsters" constructed only from S combinators
* `tests.in/test.077`: Peculiar terms from Johannes Waldemann's doctoral thesis
* `tests.in/test.079`: More Ogre, eater or Egocentric Bird terms, all with BWICK combinators
* `tests.in/test.080`: Arithmetic from "Lambda-Calculus and Combinators, an Introduction", Hindley and Seldin, chapter 4
* `tests.in/test.081`: Amusing cyclic terms
* `tests.in/test.085`: Section 24, "Birds that can do arithmetic" from "To Mock a Mockingbird"
* `tests.in/test.087`: Klein Fourgroup, from  Barendregdt's 1988 "Juggling With Combinators"
* `tests.in/test.088`: Mayer Goldberg's D numerals
