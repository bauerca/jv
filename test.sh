#!/bin/bash

# Compile it
#gcc -D JVDEBUG -o jv jv_cli.c
gcc -D JVBUF=1 -o jv jv_cli.c
#gcc -D JVBUF=1 -D JVDEBUG -o jv jv_cli.c

ONLY=""

# Run a test.
#
#   format: run <test-name> <json> <path> <expected-value>

function run {
    if [[ -z "$ONLY" || "$1" == "$ONLY" ]]; then
        OUTPUT=$(printf "$2" | ./jv "$3")
        if [[ "$OUTPUT" != "$4" ]]; then
            echo "$1 failed"
            echo "  expected: $4"
            echo "  output: $OUTPUT"
            exit 1
        fi
        echo "$1 OK"
    fi
}

run "Simplest" '{"hi":1}' 'hi' '1'
run "Incomplete stream 1" '{"hi":{"hello":1}}' 'hi' '{"hello":1}'
run "Incomplete stream 2" '{"hi":{"hello":1}' 'hi' '{"hello":1}'
run "Incomplete stream 3" '{"hi":{"hello":1' 'hi' '{"hello":1'
run "Skip key" '{"a":1, "b":2}' 'b' '2'
run "Nested" '{"a":{"b":2}}' 'a.b' '2'
run "Triple nested" '{"a":{"b":{"c":2}}}' 'a.b.c' '2'
run "Brackets" '{"a":{"b":2}}' 'a["b"]' '2'
run "Array" '[1, 2]' '[0]' '1'
run "Array skip" '[1, 2]' '[1]' '2'
run "Nested array" '{"a":[1, 2]}' 'a[1]' '2'
run "Screwy" '{"a":{1, 2]}' 'a[1]' ''
run "Null" '[0, 1, null]' '[2]' 'null'
run "String" '[0, 1, "hi"]' '[2]' 'hi'
run "Escape quote" '[0, 1, "hi\\"hi"]' '[2]' 'hi\"hi'
run "Skip string" '[0, "hi", "there"]' '[2]' 'there'
