#!/bin/bash
lines=17
read -p "Please enter the installation directory:" INSTALLDIR
SETUPDIR=`echo $INSTALLDIR|sed $'s/'\''//g'`
echo $SETUPDIR
tail -n +$lines "$0" > /tmp/ijjs.tar.gz
tar zxvf /tmp/ijjs.tar.gz -C /tmp
mv /tmp/data /tmp/ijjs
cp -r /tmp/ijjs $SETUPDIR
rm -rf /tmp/ijjs.tar.gz
rm -r /tmp/ijjs
set -e
echo "export IJJS=\"$SETUPDIR/ijjs\"" >> ~/.bash_profile
echo 'export PATH=$IJJS:$PATH' >> ~/.bash_profile
echo 'source ~/.bash_profile' >> ~/.bashrc
exit 0
