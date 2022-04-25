#!/usr/bin/bash
cd ./include/web/
for X in index.htm favicon.ico cw.png dark.min.js minstyle.io.css minstyle.io.css.map; do
    #gzip file
    /usr/bin/gzip -k -9 $X
    #convert.gz to .h
    /usr/bin/xxd -c 128 -i $X.gz | grep -v '_len = ' | sed -e 's/unsigned char/const uint8_t/;' | sed -e 's/ = / PROGMEM = /;' > $X.h
    #get rid of gzip
    /usr/bin/rm $X.gz
    echo "rebuilt $X.h"
done