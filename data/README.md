minstyle 2.0.1 https://minstyle.io/docs/get-started/installation
source: https://cdn.jsdelivr.net/npm/minstyle.io@2.0.1/dist/css/minstyle.io.min.css

minstyle darkmode switcher 0.0.1 https://minstyle.io/docs/Layout/dark
source: https://cdn.jsdelivr.net/npm/dark-mode-switcher@0.0.1/dist/dark.min.js

these files need to be gzip'd and uploaded to the LittleFS partition. FTP is easiest.


```sh
$ gzip -k -9 cw.png dark.min.js favicon.ico index.htm minstyle.io.css
$ ftp -n <<EOF
open stepper.local
user admin minad
put cw.png.gz
put dark.min.js.gz
put favicon.ico.gz
put index.htm.gz
put minstyle.io.css.gz
EOF
```