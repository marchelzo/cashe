### cashe

It's potentially kind of useful for making aliases... and stuff.

#### Installation

```
$ git clone http://github.com/marchelzo/cashe.git
$ sudo make install
```

#### Etymology

The name is a pun on the word `case`, since `cashe` programs have
a syntax similar to that of the `switch` / `case` statements found
in many popular programming languages. The `s` has become `sh`,
since `cashe` passes commands to `$SHELL`.

#### Usage

A `cashe` file looks like this:

```bash
#!/usr/local/bin/cashe 4

loop:
    for i in $(seq 5); do
        echo $i
    done

g:
    echo "This is a git alias"
    git $ARGS

echo "None of the cases matched"
```

If this file was called `test`, and it was executable, then I could use it in the following way:

```
$ ./test loop
1
2
3
5

$ ./test blah
None of the cases matched

$ ./test g branch
This is a git alias
* master
```

#### A slightly more complex example

`movies`
```bash
#!/usr/local/bin/cashe 4

random:
    cat ~/.watch_list | sort -R | head -n 1
watch:
    echo "$ARGS" >> ~/.watch_list
watched:
    echo "$ARGS" >> ~/.watched_list
    cat ~/.watch_list | grep -v "$ARGS" | sponge ~/.watch_list
list:
    _:
        cat -n ~/.watch_list
    seen:
        cat -n ~/.watched_list
    all:
        cat ~/.watch_list <(sed 's/$/ [SEEN]/' ~/.watched_list) | sort | cat -n
```

Now you've got the following commands:
```
movies watch MOVIE -- adds MOVIE to your watch list
movies watched MOVIE -- removes MOVIE from your watch list and adds it to your watched list
movies list -- lists the movies that are on your watch list
movies list seen -- lists the movies that are on your watched list
movies list all -- lists movies that are on either list, marking those on your watched list as [SEEN]
movies random -- lists a random movie from your watch list
```

#### What does the `4` signify?
The first (and mandatory) argument to `cashe` is the amount (in spaces) that each line is indented with
respect to its parent (i.e., the case that must be matched in order for it to be reached).
