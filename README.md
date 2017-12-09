# c2fp
A poorly written library for those who want to convert closures (like `std::function` and friends) to raw function pointers in a questionable, unsafe, and unportable way. System V x86\_64 only.
## Features
What you see is what you get.
## Usage
Including `c2fp.hpp`...

```cpp
int i = 42;
int j = 13;
auto f = [i](int x) { cout << x + i << endl; };
auto g = [j](int x) { cout << x - j << endl; };
closure<void, int> c(f);
closure<void, int> d(g);
void (*h)(int) = c.function_pointer();
void (*k)(int) = d.function_pointer();
function_that_takes_a_plain_function_pointer(h);
function_that_takes_a_plain_function_pointer(k);```
## Development
If you know of ways to make c2fp even more questionable, feel free to submit a pull request. If you know of a more legitimate and safer way to do this, this repository might be too ugly and poorly written for you to work on.
### Dependencies
System V x86\_64.

Oh, you mean libraries? Nah.
### Building
Gently type `make` in your terminal and hit Ctrl+M (on IBM compatible keyboards) or Control+M (on Apple keyboards), or the Return key if you have functional one. Hopefully a file called `libc2fp.so` or `libc2fp.dylib` will gently manifest itself after a moment or two.
## P.A.Q. (Possibly Asked Questions)
#### Why is your code so ugly?
Hey, at least it works (or so I hope).
#### Why are you calling your code ugly? It looks okay to me.
Because C++ isn't exactly pretty, unlike me =^=
#### Your code looks pretty well written. Why are you calling it ugly?
Just in case it is.
## Contact
Don't touch me =^=  
You can message me, though o3o
