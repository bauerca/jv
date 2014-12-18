# JSON Value

A C library and command line interface to get values from JSON streams.  It
operates entirely on the stack, and therefore keeps a small and predictable
memory footprint while parsing arbitrarily large streams.

```
> echo '{"dogs":[{"name":"oscar","breed":"golden"}]}' | jv 'dogs[0].breed'
golden
```

## Installation

There are absolutely no dependencies beyond the C standard libs. Go ahead
and:

```
> git clone https://github.com/bauerca/jv.git
> cd jv
> gcc -o jv jv_cli.c
```

There are a few optional configuration options set by macro definitions
[described below](#configuration).

## Usage

jv uses [JSON path strings](#json-path-string) to extract data from JSON [FILE
streams](http://www.cplusplus.com/reference/cstdio/FILE/). It can be used from
the [command line](#command-line-interface) by [compiling](#installation)
`jv_cli.c` or as a [library](#library-api) by dropping `jv.h` and `jv.c` into
your project.

### JSON path string

A JSON path string (JPS) is a superset of the commands used in Javascript to
get data from plain-old-javascript-objects (POJOs). Therefore, a valid
JPS would be:

```
dogs[0].breed
```

which means: from the top-level object, get the "breed" attribute of the
first dog object in the "dogs" array. In other words, the JSON is expected
to look like, for example:

```js
{
    "dogs": [
        {
            "name": "oscar",
            "breed": "golden"
        }
    ]
}
```

### Command line interface

The provided command line interface is quite simple. There are two ways
to call jv: on the receiving end of a unix pipe (jv reads from stdin),
or with a filename argument

```
jv [<filename>] <json-path-string>
```

For example, with a filename

```
> jv ./animals.json dogs[0].breed
```

and without

```
> cat ./animals.json | jv dogs[0].breed
> jv dogs[0].breed < ./animals.json
```

Without dynamic memory, jv cannot look ahead for invalid JSON before piping
what you asked for to stdout. It will, however, exit unsuccessfully when
bad JSON is detected. For example, given the busted JSON:

```js
{
  "a": {
    "b": {"answer": 42
```

the command `jv a.b` would print `{"answer": 42` (although, the exit
code would be nonzero, indicating an error).

Try out command-line jv on [some big
JSON](https://github.com/zeMirco/sf-city-lots-json); inside jv directory, after
compiling:

```
> curl -L -o sf.zip "https://github.com/zeMirco/sf-city-lots-json/archive/master.zip"
> unzip sf.zip
> ./jv features[67000] < sf-city-lots-json/citylots.json
```

Better yet, **stream it**:

```
> curl -N -s "https://raw.githubusercontent.com/zemirco/sf-city-lots-json/master/citylots.json" | ./jv features[100]
```

Only 60 kb were downloaded, rather than a 21 mb zip! Of course, this is useless
if you need the last element, but hey. (Actually, you may not want to save MBs
to disk, in which case jv *would* be useful for grabbing the last element.)


### Library API

TODO. Write this.


### Configuration

The behavior of jv is slightly configurable using macro definitions;
here are the descriptions of the available symbols.

#### JVBUF

The size of the buffer (in bytes) used by the JSON stream parser.
(default: 256). Remember, this will be allocated once on the stack; the
stack may not be very big. GCC example:

```
> gcc -D JVBUF=1024 -o jv jv_cli.c
```

#### JVDEBUG

Does not take a value. Define this to have jv spit out all kinds of
debug messages. GCC example:

```
> gcc -D JVDEBUG -o jv jv_cli.c
```


## License

MIT
