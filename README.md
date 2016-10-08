# cl - combinatory logic interpreter

This document describes how to build and use `cl`. `cl` interprets
a programming language with a lot of similarities to various "Combinatory Logic" (CL)
formal systems. It doesn't exactly interpret any "Combinatory Logic" in that
it runs on computers with finite CPU speed and a finite memory.
Most or all formal systems fail to take these limits into account.

I started writing this interpreter as a way to test the "graph reduction"
implementation of a lambda calculator.  Along the way, I bought a copy
of Raymond Smullyan's
[To Mock A Mockingbird](http://www.amazon.com/Mock-Mockingbird-Raymond-M-Smullyan/dp/0192801422/ref=sr_1_3?ie=UTF8&amp;s=books&amp;qid=1226791982&amp;sr=8-3)
While working that book's problems,
I ended up making `cl` into a reasonably complete and usable system.

## Building and installing

Try one of these commands:

    make cc       # very generic, Solaris or *BSD
    make gnu      # all GNU, most/all linux distros
    make clang    # clang compiler

For most or all Linux distros with development tools installed, `make gnu` should work.
The last time I had access to Solaris, `make cc` worked. I believe it will work on *BSD
systems.  `make clan` is for those upright citizens with Clang installed.

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
