#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jv.h"



void init_output(struct output_t *out, FILE *fp, char *mem, size_t size) {
    out->fp = fp;
    out->mem = mem;
    out->mem_pos = mem;
    out->mem_size = size;
}

int capture(const char *string, size_t len, struct output_t *out) {
    size_t num;

    if (out == NULL) return OK;

    if (out->fp != NULL) {
        num = fwrite(string, JVBYTE, len, out->fp);
        if (num != len) {
            //fprintf(stderr, "Error writing to stream.\n");
            //exit(STREAM_WRITE_ERROR);
            return STREAM_WRITE_ERROR;
        }
    }

    if (out->mem != NULL) {
        // Write as much to mem as we can from string.
        num = out->mem_size - (out->mem_pos - out->mem); // space left
        if (len < num) num = len;
        if (num > 0) {
            memcpy(out->mem_pos, string, num);
            out->mem_pos[num] = '\0';
            out->mem_pos += num;
        }
    }

    return OK;
}


int get_key(const char *path, struct key_t *key) {
#ifdef JVDEBUG
    fprintf(stdout, "Getting key\n");
#endif
    const char *chp;

    // First determine key type and value.
    switch (path[0]) {
        case '\0': {
            return EMPTY_PATH_STRING;
        }
        case '[': {
            if (path[1] >= '0' && path[1] <= '9') {
                key->type = ARRAY_INDEX;
                key->value = path + 1;
            }
            else if (path[1] == '"') {
                key->type = BRACKETED_NAME;
                key->value = path + 2;
            }
            else return BAD_PATH_STRING;
            break;
        }
        case '.': {
            key->type = NAME;
            key->value = path + 1;
            break;
        }
        // Otherwise it's a name.
        default: {
            key->type = NAME;
            key->value = path;
        }
    }

    // Now determine key end and beginning of next key.
    switch (key->type) {
        case ARRAY_INDEX: {
            chp = strchr(key->value, ']');
            if (chp == NULL) {
                return BAD_PATH_STRING;
            }
            key->len = chp - key->value;
            key->next = chp + 1;
            break;
        }
        case NAME: {
            chp = strpbrk(key->value, "[.");
            key->next = (chp == NULL) ? path + strlen(path) : chp;
            key->len = key->next - key->value;
            break;
        }
        case BRACKETED_NAME: {
            // ']' may be in the quoted string. Find the next one that
            // has a '"' in front of it.
            chp = key->value;
            while (1) {
                chp = strchr(chp, '"');
                if (chp == NULL) {
                    return BAD_PATH_STRING;
                }
                if (chp[-1] != '\\') break;
                chp++;
            }
            if (chp[1] != ']') {
                return BAD_PATH_STRING;
            }
            key->next = chp + 2;
            key->len = chp - key->value;
            break;
        }
    }

#ifdef JVDEBUG
    fprintf(stdout, "  key: %.*s, len: %lu\n", (int)key->len, key->value, key->len);
#endif
    return OK;
}


/**
 *  Read a new chunk of data from the JSON stream. Replaces the old
 *  buffer and sets the position to NULL.
 */

int read(struct json_stream_t *stream) {
    size_t count;
    size_t buf_len;

    if (feof(stream->src)) {
        // TODO: Should we leave it up to the caller to decide what
        // to do if the file ends?
        return stream->code = END_OF_STREAM;
        //exit(0);
    }

    // Save last character from current buffer for lookbacks.
    buf_len = strlen(stream->buffer);
    if (buf_len) {
        stream->prev_char = stream->buffer[buf_len - 1];
    }

    // Get more data from the stream.
    count = fread(stream->buffer, JVBYTE, JVBUF, stream->src);
    if (count != JVBUF) {
        if (ferror(stream->src)) {
            //fprintf(stderr, "Error reading from stream");
            //exit(1);
            return stream->code = STREAM_READ_ERROR;
        }
        else if (count == 0) {
            // TODO: Should we leave it up to the caller to decide what
            // to do if the file ends?
            return stream->code = END_OF_STREAM;
            //exit(0);
        }
    }

    stream->buffer[count] = '\0';
    // Casts from array to pointer type. Subtle. See
    // http://stackoverflow.com/questions/1335786/c-differences-between-char-pointer-and-array
    stream->pos = stream->buffer;
    return OK;
}


int init_stream(struct json_stream_t *stream, FILE *src) {
    // Set up for first read.
    stream->src = src;
    // stream->pos = stream->buffer; // set in read()
    // stream->prev_char = '\0'; // set in read()
    // stream->buffer[JVBUF] = '\0'; // set in read()

    stream->buffer[0] = '\0'; // avoids set prev_char in read().
    stream->prev_char = '\0';

    return read(stream);
}


int bump(struct json_stream_t *stream, struct output_t *out) {
    capture(stream->pos, 1, out);
    if (stream->pos[1] == '\0') {
        return read(stream);
    }
    stream->prev_char = stream->pos[0];
    stream->pos++;
    return OK;
}

int search(struct json_stream_t *stream, const char *chars, struct output_t *out) {
    const char *chp;
    const char *start;
    int code;

#ifdef JVDEBUG
    fprintf(stdout, "Searching stream for: %s\n", chars);
    fprintf(stdout, "  buffer: %s\n", stream->pos + 1);
#endif

    // stream->pos[0] can never be '\0'; therefore, (pos + 1) will never
    // be out of bounds (although it may point to '\0', which is fine for
    // strpbrk).
    start = stream->pos + 1;
    while ((chp = strpbrk(start, chars)) == NULL) {
        code = capture(stream->pos, strlen(stream->pos), out);
        if (code != OK) {
            return stream->code = code;
        }
        code = read(stream);
        if (code != OK) {
            return code;
        }
#ifdef JVDEBUG
        fprintf(stdout, "  buffer: %s\n", stream->pos);
#endif
        start = stream->pos;
    }

#ifdef JVDEBUG
    fprintf(stdout, "  found at: %s\n", chp);
#endif

    // Dump from current position to chp (exclusive).
    // If we read from the stream,
    // the current position points to buffer[0]. Good.
    code = capture(stream->pos, chp - stream->pos, out);
    if (code != OK) {
        return stream->code = code;
    }

    if (chp > stream->buffer) {
        stream->prev_char = *(chp - 1); // Dangerous, but we know.
    }
    stream->pos = chp;

    return OK;
}


int traverse_collection(struct json_stream_t *stream, char open, int count, struct output_t *out) {
    char match_chars[4] = {'\0', '\0', '"', '\0'};
    char ch = 0;
    char close;
    int env = OBJECT;

    match_chars[0] = open;
    close = match_chars[1] = (open == '{') ? '}' : ']';

    if (stream->pos[0] == close) count--;

    while (count > 0) {
        if (search(stream, match_chars, out) != OK) {
            return stream->code;
        }
        ch = stream->pos[0];

        if ((ch == '"') && (stream->prev_char != '\\')) {
            // Change environment to/from STRING.
            env = (env == STRING ? COLLECTION : STRING);
        }
        else if (ch == open && env != STRING) count++;
        else if (ch == close && env != STRING) count--;
    }

    return bump(stream, out);
}

int traverse_string(struct json_stream_t *stream, struct output_t *out) {
    do {
        if (search(stream, "\"", out) != OK) return stream->code;
    }
    while (stream->prev_char != '\\');
    return bump(stream, out);
}

int traverse_number(struct json_stream_t *stream, struct output_t *out) {
    // Search for all things that could possibly follow a
    // number.
    return search(stream, "\n\t\r }],", out);
}

int traverse_null(struct json_stream_t *stream, struct output_t *out) {
                                                      // n
    if (bump(stream, out) != OK) return stream->code; // u
    if (bump(stream, out) != OK) return stream->code; // l
    if (bump(stream, out) != OK) return stream->code; // l
    if (bump(stream, out) != OK) return stream->code; // *
    return OK;
}


/**
 *  Skip functions. Pass over values without capturing their contents.
 */

int skip_collection(struct json_stream_t *stream) {
    return traverse_collection(stream, stream->pos[0], 1, NULL);
}

int skip_string(struct json_stream_t *stream) {
    return traverse_string(stream, NULL);
}

int skip_number(struct json_stream_t *stream) {
    return traverse_number(stream, NULL);
}

int skip_null(struct json_stream_t *stream) {
    return traverse_null(stream, NULL);
}

/**
 *  Stream must point to first character of value. On output,
 *  stream points to character just beyond value.
 */

int skip_value(struct json_stream_t *stream) {
    switch (stream->pos[0]) {
        case '{':
        case '[': return skip_collection(stream);
        case '"': return skip_string(stream);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': return skip_number(stream);
        case 'n': return skip_null(stream);
        default: return stream->code = NOT_AT_VALUE;
    }
}



const char *scan_object(struct json_stream_t *stream, const char *path) {
#ifdef JVDEBUG
    fprintf(stdout, "Scanning object\n");
#endif
    // Jump to the first '"' or closing '}'.
    if (search(stream, "\"}", NULL) != OK) return NULL;

    while (stream->pos[0] != '}') {
        path = scan_pair(stream, path);
        if (path == NULL || path[0] == '\0') {
            // Error, or complete and utter success.
            return path;
        }
        // Stream now points to char after skipped value.
        // This is one of: ",}\s". If not '}', run search for
        // next key or closing '}'.
        if (stream->pos[0] != '}') {
            if (search(stream, "\"}", NULL) != OK) return NULL;
        }
    }

    // Stream points to '}'. Go just beyond the object, as ya do.
    if (bump(stream, NULL) != OK) return NULL;

    // Sanity check that input path = subpath?
    return path;
}


const char *scan_array(struct json_stream_t *stream, const char *path) {
#ifdef JVDEBUG
    fprintf(stdout, "Scanning array\n");
#endif
    struct key_t key;
    long int index;
    long int i;
    int code;
    char ch;

    code = get_key(path, &key);
    if (code != OK) {
        stream->code = code;
        return NULL;
    }

    if (key.type != ARRAY_INDEX) {
        return path;
    }

    index = strtol(key.value, NULL, 10);
    if (index == 0L && key.value[0] != '0') {
        stream->code = ARRAY_INDEX_ERROR;
        return NULL;
    }

    // Start the search!
    for (i = 0; i <= index; i++) {
        // Jump to value or closing ']'.
        if (search(stream, "]{[\"0123456789-n", NULL) != OK) {
            return NULL;
        }
        ch = stream->pos[0];

        if (ch == ']') return path;
        else if (i == index) {
            return scan_value(stream, key.next);
        }
        else if (skip_value(stream) != OK) {
            return NULL;
        }
    }

    // If we get out of the loop, we must skip the remaining
    // elements. Stream is currently pointing to char after
    // mismatched value.

    if (traverse_collection(stream, '[', 1, NULL) != OK) {
        return NULL;
    }

    return path;
}



const char *scan_value(struct json_stream_t *stream, const char *path) {
#ifdef JVDEBUG
    fprintf(stdout, "Scanning value\n");
#endif
    if (path[0] == '\0') return path;

    switch (stream->pos[0]) {
        case '{': return scan_object(stream, path);
        case '[': return scan_array(stream, path);
        default: return path;
    }

    return NULL;
}


const char *scan_key(struct json_stream_t *stream, const char *path) {
#ifdef JVDEBUG
    fprintf(stdout, "Scanning key, path: %s\n", path);
#endif
    struct key_t path_key;
    int code;
    size_t i;

    code = get_key(path, &path_key);
    if (code != OK) {
        stream->code = code;
        return NULL;
    }

    // Go to first character of object key name.
    if (bump(stream, NULL) != OK) return NULL;

    for (i = 0; i < path_key.len; i++) {
        if (path_key.value[i] != stream->pos[0]) {
            // Mismatch. Advance stream to closing quote.
#ifdef JVDEBUG
            fprintf(stdout, "  mismatch (path: %c, object: %c)\n", path_key.value[i], stream->pos[0]);
#endif
            while (stream->pos[0] != '"' || stream->prev_char == '\\') {
                if (search(stream, "\"", NULL) != OK) return NULL;
            }
            return path;
        }
        if (bump(stream, NULL) != OK) return NULL;
    }

    return path_key.next;
}


const char *scan_pair(struct json_stream_t *stream, const char *path) {
#ifdef JVDEBUG
    fprintf(stdout, "Scanning pair\n");
#endif
    const char *subpath;

    subpath = scan_key(stream, path);
    if (subpath == NULL) return NULL;

    // Always advance to value.
    if (search(stream, VALUE_TIPS, NULL) != OK) {
        return NULL;
    }

    if (subpath == path) {
        // No key match. Skip the value.
#ifdef JVDEBUG
        fprintf(stdout, "  key mismatch (%s), skipping value.\n", path);
#endif
        if (skip_value(stream) != OK) return NULL;
        return path;
    }
    else {
        return scan_value(stream, subpath);
    }
}

/**
 *  Pipe functions. Traverse values while capturing their contents.
 */

int pipe_collection(struct json_stream_t *stream, struct output_t *out) {
#ifdef JVDEBUG
    fprintf(stdout, "Piping collection\n");
#endif
    return traverse_collection(stream, stream->pos[0], 1, out);
}

int pipe_string(struct json_stream_t *stream, struct output_t *out) {
#ifdef JVDEBUG
    fprintf(stdout, "Piping string\n");
#endif
    // Avoid capturing opening quote. Bump to first char. Check it b/c
    // search won't include it.
    if (bump(stream, NULL) != OK) return stream->code;
    if (stream->pos[0] == '"') return OK; // Empty string.

    do {
        if (search(stream, "\"", out) != OK) return stream->code;
    }
    while (stream->prev_char == '\\');

    return bump(stream, NULL);
}

int pipe_number(struct json_stream_t *stream, struct output_t *out) {
#ifdef JVDEBUG
    fprintf(stdout, "Piping number\n");
#endif
    return traverse_number(stream, out);
}

int pipe_null(struct json_stream_t *stream, struct output_t *out) {
#ifdef JVDEBUG
    fprintf(stdout, "Piping null\n");
#endif
    return traverse_null(stream, out);
}

int pipe_value(struct json_stream_t *stream, struct output_t *out) {
    switch (stream->pos[0]) {
        case '{':
        case '[': return pipe_collection(stream, out);
        case '"': return pipe_string(stream, out);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': return pipe_number(stream, out);
        case 'n': return pipe_null(stream, out);
        default: return stream->code = NOT_AT_VALUE;
    }
}


int main(int argc, char **argv) {
    FILE *fp;
    const char *path;
    struct json_stream_t stream;
    struct output_t out;

    if (argc == 2) {
        // Stream from stdin
        fp = stdin;
        path = argv[1];
    }
    else if (argc == 3) {
        // Stream in a file.
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "Error opening file %s.\n", argv[1]);
            exit(2);
        }
        path = argv[2];
    }
    else {
        fprintf(stderr, "Usage: ga <file> <attr>\n");
        fprintf(stderr, "Usage: jv main.browser.whatever < config.json\n");
        exit(1);
    }

    if (init_stream(&stream, fp) != OK) {
        fprintf(stderr, "Problem initializing stream.\n");
        exit(1);
    }

    path = scan_value(&stream, path);
    if (path == NULL) {
        // Error bomb. For now, just exit with code.
        exit(stream.code);
    }
    if (path[0] != '\0') {
        // Successful scan, no match. Return nothing.
        exit(OK);
    }

    // Match! Let's pipe.
    init_output(&out, stdout, NULL, 0);
    if (pipe_value(&stream, &out) != OK) {
        exit(stream.code);
    }

    exit(OK);
}
