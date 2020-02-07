#!/usr/bin/bash

echo "*** Remove utf-8 (double quote)"
for i in `ls *.xml`
do
    echo $i
    sed -i 's/encoding="utf-8"//' $i
done

echo
echo "*** Remove UTF-8"
for i in `ls *.xml`
do
    echo $i
    sed -i 's/encoding="UTF-8"//' $i
done

echo
echo "*** Remove utf-8 (single quote)"
for i in `ls *.xml`
do
    echo $i
    sed -i "s/encoding='utf-8'//" $i
done

echo
echo "*** Remove UTF-8 (single quote)"
for i in `ls *.xml`
do
    echo $i
    sed -i "s/encoding='UTF-8'//" $i
done

echo
echo "*** Replace single quote version"
for i in `ls *.xml`
do
    echo $i
    sed -i "s/version='1.0'/version=\"1.0\"/" $i
done

echo
echo "*** Removing spaces"
for i in `ls *.xml`
do
    echo $i
    sed -i "s/xml.*version.*=.*\"1.0\".*?>/xml version=\"1.0\" ?>/" $i
done
