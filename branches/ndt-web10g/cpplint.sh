#!/bin/bash

for f in src/*.[ch]; do
  ./cpplint.py \
      --filter=-legal/copyright,-readability/casting,-build/include_order,-runtime/threadsafe_fn,-build/include \
      $f
done
