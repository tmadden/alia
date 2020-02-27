#!/bin/bash
# Build the documentation in the "www" directory.
# Note that the asm-dom examples must be built in examples/asm-dom/build before
# running this script.
set -e
mkdir -p www
cd www
rm -rf *
cp ../docs/* .
unzip favicon.zip && rm *.zip
cp ../examples/asm-dom/build/*.js ../examples/asm-dom/build/*.wasm .
cp ../examples/asm-dom/alia.hpp .
dot -Tsvg -o data-graph.svg data-graph.dot && rm data-graph.dot
cd ..
python scripts/process-docs.py
