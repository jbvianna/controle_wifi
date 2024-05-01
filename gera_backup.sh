#!/bin/bash

# Gera Backup do Sistema em uma nova pasta
# Uso: ./gera_backup.sh <nome_dir>
mkdir $1
cp CMakeLists.txt $1
cp README.md $1
cp *.csv $1
cp sdkconfig.ci $1
cp sdkconfig.defaults $1
mkdir $1/main
cp main/*.c $1/main/
cp main/*.h $1/main/
cp main/*.txt $1/main/
cp main/*.html $1/main/
cp main/*.css $1/main/
cp main/*yml $1/main/
cp main/*.projbuild $1/main/
cp gera_backup.sh $1
cp -r flash_data $1/
cp -r client $1/
cp -r doc $1/

