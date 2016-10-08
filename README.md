# cl - combinatory logic interpreter

This document describes how to build and use `cl`. `cl` interprets
a programming language with a lot of similarities to various "Combinatory Logic" (CL)
formal systems. It doesn't exactly interpret any "Combinatory Logic" in that
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

The <kbd>cl</kbd> interpreter uses the conventional square-bracket
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

