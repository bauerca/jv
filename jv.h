
#define BUF_SIZE 2048
#define BUF_CHARS (BUF_SIZE - 1)
#define BYTE sizeof(char)

enum return_code_t {
    OK,
    END_OF_BUFFER,
    OUT_OF_BOUNDS,
    END_OF_STREAM,
    STREAM_READ_ERROR,
    STREAM_WRITE_ERROR,

    ARRAY_INDEX_ERROR,

    NOT_AT_VALUE,

    // Returned when a scan_key function does not match the
    // path string.
    KEY_MISMATCH,

    BAD_PATH_STRING,
    EMPTY_PATH_STRING,
    WRITE_ERROR
};


enum key_type_t {
    ARRAY_INDEX,
    BRACKETED_NAME,
    NAME
};

/**
 *  Data identifying a key in a path string. Initialize this using
 *  the get_key() function. See below.
 */

struct key_t {
    enum key_type_t type;

    /**
     *  Pointer to first character of key name or first digit of
     *  array index
     */

    const char *value;

    /**
     *  Length of key name or integer (not including surrounding
     *  quotes and/or brackets, if any).
     */

    size_t len;

    /**
     *  A pointer to the next key in the path string. This is set
     *  by get_key(). See below.
     */

    const char *next;
};

/**
 *  Given a path string, fill the given key_t struct with information
 *  on the location of the first key name in the path string, the type of
 *  key, (name vs array index), etc. See key_t struct definition.
 *
 *  Possible input path pointer positions are.
 *
 *      a.b["c"].d[2]
 *      ^
 *
 *      a.b["c"].d
 *       ^
 *
 *      a.b["c"].d
 *         ^
 *
 *
 *  Returns
 *
 *      - EMPTY_PATH_STRING
 *      - BAD_PATH_STRING
 *      - OK
 */

int get_key(const char *path, struct key_t *key);


/**
 *  Some JSON type ids.
 */

enum value_type_t {
    ARRAY,
    OBJECT,
    STRING,
    COLLECTION,
    NUMBER,
    NIL,
    PRIMITIVE
};

/**
 *  Characters that mark the beginning of a JSON value (i.e. an array element
 *  or object value).
 */

const char VALUE_TIPS[] = "{[\"0123456789-n";

/**
 *  Abstract different ways of capturing stream output from this
 *  library.
 */

struct output_t {
    /**
     *  If not NULL, jv will write to it.
     */

    FILE *fp;

    /**
     *  This array should hold (mem_size + 1) characters. This leaves room for
     *  the null char.  If not NULL, jv will write to it.
     */

    char *mem;

    /**
     *  This is the number of characters that will fit in the
     *  memory block declared above.
     */

    size_t mem_size;

    /**
     *  This points to the character slot that should be written
     *  next. When mem is full, this would point to the char at
     *  index mem_size.
     *
     *  Internal use only.
     */

    char *mem_pos;
};


/**
 *  Initialize an output struct. Simple mapping between function args
 *  and struct data.
 */

void init_output(struct output_t *out, FILE *fp, char *mem, size_t size);

/**
 *  Capture some string data with an output struct.
 */

int capture(const char *string, size_t len, struct output_t *out);


/**
 *  The beast. This gets passed around like grandma's apple crisp.
 *  Initialize with init_stream().
 */

struct json_stream_t {
    /**
     *  Internal
     */

    char buffer[BUF_SIZE + 1];

    /**
     *  Current position in buffer. When a new buffer is read in from the
     *  source, this points at the first character. Searching and bumping
     *  are the valid ways to move this along.
     */

    const char *pos;

    /**
     *  JSON parsing requires some look behinds because of the escape
     *  character.
     *
     *  Internal.
     */

    char prev_char;

    /**
     *  The source.
     */

    FILE *src;

    /**
     *  When a stream is passed to a function that ultimately fails, the
     *  error code is stored here so that the function is free to customize
     *  its return value.
     */

    int code;
};


/**
 *  Must call this to initialize a JSON stream with a C stream source.
 *  It will fail when there are errors reading from the given source.
 */

int init_stream(struct json_stream_t *stream, FILE *src);

/**
 *  Force the json_stream to read from the source. This does not check
 *  if the stream position is at the end.
 */

int read(struct json_stream_t *stream);

/**
 *  Move the stream position forward by one character. Automatically
 *  reads from the stream source if necessary.
 *
 *  Returns OK if successful, otherwise an error code.
 */

int bump(struct json_stream_t *stream, struct output_t *out);

/**
 *  Search the stream for characters and advance the stream to the first match.
 *  If an output struct is provided (not NULL), all data is captured up to (but
 *  not including) the match. This includes the stream position on input.
 *
 *  Returns OK if successful, otherwise an error code.
 */

int search(struct json_stream_t *stream, const char *chars, struct output_t *out);

/**
 *  Go to the end of a current object or array context.
 *
 *  On input, the stream must point to a character inside an object or array,
 *  and NOT inside a string. It is okay for the stream to point to the opening
 *  ('{' or '[') or closing ('}' or ']') character.
 *
 *  On output, the stream will point to the character past the closing '}' or
 *  ']'.
 *
 *  Parameters:
 *
 *      stream: The stream, pointing somewhere inside the object or array.
 *
 *      open: This should be '{' or '['.
 *
 *      count: This must be the number of opening characters already passed.
 *      Usually, this is 1.
 *
 *      out: Pass an output struct if you would like to collect all characters
 *      traversed.
 *
 */

int traverse_collection(struct json_stream_t *stream, char open, int count, struct output_t *out);

/**
 *  Infer from description of traverse collection above.
 */

int traverse_string(struct json_stream_t *stream, struct output_t *out);
int traverse_number(struct json_stream_t *stream, struct output_t *out);
int traverse_null(struct json_stream_t *stream, struct output_t *out);

/**
 *  Derivatives of the traverse functions, without capturing output.
 */

int skip_collection(struct json_stream_t *stream);
int skip_string(struct json_stream_t *stream);
int skip_number(struct json_stream_t *stream);
int skip_null(struct json_stream_t *stream);

/**
 *  Generic form for above skip functions; basically a map from stream position
 *  to one of the above.
 *
 *  On input, stream must point to first character of value. On output, stream
 *  points to character just beyond value.
 */

int skip_value(struct json_stream_t *stream);


/**
 *  Scan functions search the JSON stream for a match to a value path.
 *  The value path has the same format as one would use in Javascript to
 *  access a value inside an object or array. For example, if the JSON
 *  data is a mapping between animal types and instances:
 *
 *      dogs[0]["breed"]
 *
 *  would extract the breed of the first dog. Equivalently,
 *
 *      dogs[0].breed
 *
 *  A scan function returns an empty string if it was able to match the
 *  entire path. In this case, the stream will point to the first character
 *  of the matched value.
 *
 *  If the value was not found and no errors occurred, the given path is
 *  returned and the stream points to the character immediately following
 *  the scanned value.
 *
 *  If an error occurs, scan functions return NULL and set the code parameter
 *  the json_stream struct.
 */


/**
 *  On input, stream should be pointing to opening quote of object key.
 *
 *  On output, if the path head was matched by this key, the stream points to the
 *  first character of the value, ready for piping, and the returned path is an
 *  empty C-string.
 *
 *  If the path and object keys do not match, the stream will skip
 *  ahead and point to the first char after the associated value.
 */

const char *scan_pair(struct json_stream_t *stream, const char *path);

/**
 *  Stream points to opening quote of a JSON object key.
 *
 *  On output, the stream points to the closing quote of the object key
 *  (for both matches and mismatches).
 *
 *  Returns the truncated subpath if the object key matched the head
 *  of the given value path, or the given path if there was no match.
 *  NULL if there was an error.
 */

const char *scan_key(struct json_stream_t *stream, const char *path);


/**
 *  Scan all the keys in the object pointed to by buffer (buffer
 *  points to the opening '{') looking for a match.
 *
 *  On output, if a match was found in the subtree of this object,
 *  the stream points to the first character of the matched value
 *  (see scan_pair). Otherwise, the stream points to the first
 *  character past the object.
 *
 *  Returns the given path (no match), the empty string (matchness!),
 *  or NULL (error bomb).
 */

const char *scan_object(struct json_stream_t *stream, const char *path);


/**
 *  Scan the array until we land on a value at the array index specified in the
 *  path. On input, the stream should point to the opening '['.
 *
 *  On output, if the matching subtree of the element was found, the stream
 *  points to the first character of the matched value.  Otherwise, the stream
 *  points to the first character past ']' of this array.
 *
 *  Returns the given path (no match), the empty string (matchness!), or NULL
 *  (error bomb).
 */

const char *scan_array(struct json_stream_t *stream, const char *path);

/**
 *  Mapping from stream position to more specific scan function (see above).
 *
 *  Here is where we switch from scanning the JSON to piping
 *  a value if we have matched the path string.
 *
 *  On input, stream must point to first character of value.
 *  On output, stream points to first character after value.
 */

const char *scan_value(struct json_stream_t *stream, const char *path);
