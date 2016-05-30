#!/bin/bash

./test-generator.py

gcc -c test_file*
gcc test_file*.o -o test_file
