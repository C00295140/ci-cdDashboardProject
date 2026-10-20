#!/bin/sh
cd "$(dirname "$0")" || exit 1
if [ ! -x build/pipeline-studio ]; then
    printf 'Build the project first with make setup and make.\n'
    exit 1
fi
exec ./build/pipeline-studio
