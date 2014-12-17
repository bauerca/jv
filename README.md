# JSON Value

A C library and command line interface to get values from JSON
streams.

```bash
> echo '{"dogs":[{"name":"oscar","breed":"golden"}]}' > jv 'dogs[0].breed'
golden
```

## Installation

There are absolutely no dependencies beyond some C standard libs. Go ahead
and

```
> git clone https://github.com/bauerca/jv.git
> cd jv
> gcc -o jv jv.c
```

if you like.


## License

MIT
