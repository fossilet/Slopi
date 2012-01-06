#! /bin/bash

# Generate test file for slopi
# Since Jan 6 2012

rm testfile
echo -n 3. >> testfile
seq -s '' 1 9 | tr -d '\n' >> testfile
seq -s '' 10 2 99 | tr -d '\n' >> testfile
seq -s '' 100 3 999 | tr -d '\n' >> testfile
