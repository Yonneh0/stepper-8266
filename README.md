ESP8266 Stepper Controller and Tester

Currently, this project is somewhere before PRE-RELEASE. It was last recorded as being in the "Hold my beer, watch this" phase.

Code is still proof of concept. It is NOT hardened in any way. It is currently blindly open.

Requirements:
`DRV8825 Stepper Motor Driver Carrier` https://www.amazon.com/DRV8825-Stepper-Driver-Module-Printer/dp/B07XF2LYC8
`ESP-12S, ESP8266MOD, or similar` https://www.amazon.com/ESP-12S-ESP-12F-Upgrade-ESP8266-Wireless/dp/B07VF7B9XB
`Power` (3.3v 250ma, and 8.2-45V 0-4.4a for Stepper Motor)

example screenshot in misc/stepper-8266.png

minstyle 2.0.1 https://minstyle.io/docs/get-started/installation
source: https://cdn.jsdelivr.net/npm/minstyle.io@2.0.1/dist/css/minstyle.io.min.css

minstyle darkmode switcher 0.0.1 https://minstyle.io/docs/Layout/dark
source: https://cdn.jsdelivr.net/npm/dark-mode-switcher@0.0.1/dist/dark.min.js

The website files in /data/ need to be gzip'd and uploaded to the LittleFS partition. FTP is easiest.
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