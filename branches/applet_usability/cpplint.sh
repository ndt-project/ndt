#!/bin/bash

./cpplint.py \
    --filter=-legal/copyright,-readability/casting,-build/include_order,-runtime/threadsafe_fn,-build/include \
    src/*.[ch]
