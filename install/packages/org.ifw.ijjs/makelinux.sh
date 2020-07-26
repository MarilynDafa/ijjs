#!/bin/bash
cd $(dirname $0)
tar -czf ijjs.tar.gz data
cat data/runlinux.sh ijjs.tar.gz > ijjs.run
