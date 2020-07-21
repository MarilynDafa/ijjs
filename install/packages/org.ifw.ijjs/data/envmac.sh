#!/bin/bash

basepath=$(
    cd $(dirname $0)
    pwd
)
set -e

echo "\nexport IJJS=\"${basepath}\"" >> ~/.bash_profile
echo 'export PATH=$IJJS:$PATH' >> ~/.bash_profile
echo 'source ~/.bash_profile' >> ~/.zshrc
source ~/.bash_profile
