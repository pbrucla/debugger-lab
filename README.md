# Debugger Lab
## Dependencies
You need to have `meson` and `ninja` installed, and optionally `clang-format` for code formatting.

## Usage
A helper script has been provided. You can intiailize the build directory with `./helper init`, and build with `./helper build`. You can also format your code with `./helper format`.

A small, statically-linked non-PIE executable has also been provided under the name `test`. You can run with `./helper run ./test`.

## Activity
Fill in all of the `TODO:` blanks in [`dbg.cpp`](dbg.cpp).

The recommended order to fill out the TODOs is:
1. The two in `spawn_process`. After filling these out correctly you should immediately see `Read a value of: 1234567812345678` when runinng `./helper run ./test`.
2. The two in `continue_process`. After filling these out correctly you should see that all 3 prints happen before the `Hit breakpoint.` message.
3. The one in `inject_breakpoint`. After filling this out correctly, you're done.

## Finished Product
If you've done everything correctly, when running `./helper run ./test`, you should immediately see:
```
Read a value of: 1234567812345678
First print
Hit breakpoint. Press ENTER to continue.
```
Then, when you press enter, you should immediately see `Second print`, and then after a 1 second delay, `Third print: 1234567812345678`.

Afterwards, it should immediately display `Got exit code 0.`.
