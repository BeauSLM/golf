# Info

Name: Beau McCartney
UCID: 30095634
Email: beau.mccartney1@ucalgary.ca
Repo URL: //gitlab.cpsc.ucalgary.ca/beau.mccartney1/golf

# Requested area of feedback:

Simplicity and readability of the code. Investing time into improving these will help me
reduce the friction involved in completing this project. However, any and all remarks
that come to mind are welcome.

# Self-assessment

My mark for myself: 8/8

## Evidence:

Firstly, please find `run.output` (output of the provided run script) in the root of the repo.

### 1. Tool (milestone) properties

My lexing error and warning messages match those of the reference compiler exactly. This means
they provide the correct line number, name or print out the specific character that caused an
error, and give specific context to help a user find the problem. One example is the error
I throw when a single '&' is encountered. This error states that bitwise AND is not supported
in`golf`.

Additionally, my compiler tokenizes all the test inputs correctly, doesn't crash or exit in an
uncontrolled manner, and rejects incorrect input (e.g. single '&'). This is quite easy to
verify, as my output is nearly identical to that of the reference compiler (all the differences
being completely allowed). In fact, I used a diff tool (meld) to check my compiler against the
reference compiler. Please see the included run.output mentioned above.

### 2. Development practices

In the time I've worked on this, I've made a point of breaking my commits into small pieces, and
to commit quite often. At the time of this writing, the repo has 76 commits. Each commit corresponds
to finishing a small task, to make usage of tools such as `git bisect` and `git revert` easier.

I've also ensured that my commits have highly informative commit messages that concisely and accurately
describe what was accomplished in the given commit. I also often included detailed bodies providing
more information in my commit messages. One such example is the message for commit 6a462d18ff. The
commit message reads as follows:

```diff
commit 6a462d18ff4c38099c10a16523e4061332e72314
Author: Beau McCartney <beau@beaumccartney.xyz>
Date:   Fri Jan 27 18:55:37 2023 -0700

    bring all of `result`'s members into scope

    I've prefixed the identifier of each reference with "result_" to
    indicate that the identifier refers to a member of result
```

### 3. Code qualities

#### Readability:

I've followed some useful conventions to greatly improve the readability of my code:
- everything inside `()` is bookended by whitespace
    e.g.
    ```c++
    // my code...
    while ( ( ch = getc( fp ) ) && ch != '"' )

    // could have been this
    while((ch=getc(fp))&&ch!='"')
    ```

- align repetitions
    e.g. my code:
    ```c++
    // check if token is a reserved work and act accordingly
    if      ( result_lexeme == "break"  ) result_token = TOKEN_BREAK;
    else if ( result_lexeme == "else"   ) result_token = TOKEN_ELSE;
    else if ( result_lexeme == "for"    ) result_token = TOKEN_FOR;
    else if ( result_lexeme == "func"   ) result_token = TOKEN_FUNC;
    else if ( result_lexeme == "if"     ) result_token = TOKEN_IF;
    else if ( result_lexeme == "return" ) result_token = TOKEN_RETURN;
    else if ( result_lexeme == "var"    ) result_token = TOKEN_VAR;
    //otherwise, result is a generic identifier
    else                                  result_token = TOKEN_ID;
    ```

    could have been:

    ```c++
    // check if token is a reserved work and act accordingly
    if (result_lexeme == "break") result_token = TOKEN_BREAK;
    else if (result_lexeme == "else") result_token = TOKEN_ELSE;
    else if (result_lexeme == "for") result_token = TOKEN_FOR;
    else if (result_lexeme == "func") result_token = TOKEN_FUNC;
    else if (result_lexeme == "if") result_token = TOKEN_IF;
    else if (result_lexeme == "return") result_token = TOKEN_RETURN;
    else if (result_lexeme == "var") result_token = TOKEN_VAR;
    //otherwise, result is a generic identifier
    else result_token = TOKEN_ID;
    ```

#### Modularity:

Functionality is broken down into header/c++ pairs making it easy to use
a given piece of functionality
- error.cpp and error.hpp for errors
- lex.cpp and lex.hpp for lex/unlex interface

#### Extensibility:

My lexer is structured as a state machine backed by a switch statement.
Adding new rules is as simple as adding a new case in said switch statement.

#### Consistency:

I've already mentioned certain whitespace conventions I've followed, but I
follow other conventions as well.
- variables and functions named in snake case
- function parameters named as a single, lowercase word

#### Documentation:

I've made a large effort to explain *why* I'm doing things with my comments, and only
explain *what* I'm actually doing if its not obvious. When combined with my efforts
to make the code relatively simple and readable, I can use a small amount of commenting
to convey important information concisely.
e.g. below I make clear why the var is static as its not obvious

// static to preserve value across calls.
// this allows me to inspect result's token before I set it in a given call
// to check what the previous token was
static Tokinfo result;

#### Speed:

My code runs nearly instantly on all 30 test cases. I won't claim that it's highly
optimized or performant as I haven't profiled or optimized it, but it isn't noticeably
slow.

#### Building:

Here's the output of `make clean ; make` on the cpsc servers - no errors with
-Wall and -Wextra. To build, just run `make`

```sh
$ make clean ; make # running this...

# outputs this (below)
rm -f *.o *.d golf
clang++ -o main.o -c main.cpp -Wall -Wextra -O3 -MMD -MF main.d
clang++ -o error.o -c error.cpp -Wall -Wextra -O3 -MMD -MF error.d
clang++ -o lex.o -c lex.cpp -Wall -Wextra -O3 -MMD -MF lex.d
clang++ -Wall -Wextra -O3 -o golf main.o error.o lex.o
```

### 4. Relationship with environment:

I send regular output to stdout, error/warning output to stderr, and use the appropriate
exit codes in `sysexits.h`. That is:
- EX_OK for no errors
- EX_USAGE for incorrect argument(s)
- EX_NOINPUT for unreadable input file
- EX_DATAERR for errors in the provided source file

Running the compiler is simply `./golf <your_golf_file>` - perfectly simple.

My README.md file explains everything about building my compiler including debug/release
builds. It provides short version explanations, and more detail for the curious.
