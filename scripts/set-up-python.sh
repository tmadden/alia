#!/bin/bash
# Set up a Python virtual environment so that it can build alia.
echo "Setting up Python environment in .python..."
set -x -e
virtualenv --prompt="(alia) " "$@" .venv
source .venv/bin/activate
python --version
pip install gcovr
pip install -r docs/requirements.txt
