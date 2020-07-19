#!/bin/bash

basepath=$(
    cd $(dirname $0)
    pwd
)
set -e

echo "\nexport IJJS=\"${basepath}\"" >> ~/.bash_profile
source ~/.bash_profile
