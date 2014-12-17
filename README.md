# JSON Value

A C library and command line interface to get values from JSON
streams.

```
> echo '{"dogs":[{"name":"oscar","breed":"golden"}]}' | jv 'dogs[0].breed'
golden
```

## Usage

This thing works on streams and keeps a small, mostly-predictable memory
footprint (it's all on the stack!), so there is no looking ahead to make sure
JSON is valid before piping what you asked for to stdout. For example, given
the busted JSON:

```js
{
  "a": {
    "b": {"answer": 42
```

the command `jv 'a.b'` would still print `{"answer": 42` (although, the exit
code would be nonzero, indicating an error).

Try it out on [some big JSON](https://github.com/zeMirco/sf-city-lots-json);
inside jv directory, after compiling:

```
> curl -L -o sf.zip "https://github.com/zeMirco/sf-city-lots-json/archive/master.zip"
> unzip sf.zip
> ./jv features[67000] < sf-city-lots-json/citylots.json
```

Should be quick.

## Installation

There are absolutely no dependencies beyond the C standard libs. Go ahead
and:

```
> git clone https://github.com/bauerca/jv.git
> cd jv
> gcc -o jv jv_cli.c
```

### Flags

The behavior of jv is slightly configurable using compile flags;
descriptions of the available flags follow.

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
