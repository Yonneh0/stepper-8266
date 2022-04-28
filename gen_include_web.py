import sys
import gzip

for file in ['index.htm', 'favicon.ico', 'cw.png', 'dark.min.js', 'minstyle.io.css', 'minstyle.io.css.map']:
    with open('./include/web/' + file, "rb") as f:
        raw = f.read()
    data = gzip.compress(raw)
    varname = file.replace('.', '_')

    val = ""
    val = 'const uint8_t ' + varname + '[] PROGMEM = {\n '
    j = 0
    for i in bytearray(data):
        val += " 0x%02x," % i
        j = j + 1
        if j == 127:
            val += '\n '
            j = 0
    if j == 0:
        val = val[:-2]  # prune off the newline,
    val = val[:-1]  # prune off the comma
    val += "\n};\n"

    with open('./include/web/' + varname + '.h', "w") as f:
        f.write(val)
    sys.stderr.write('Rebuilt ' + file +
                     ' as ./include/web/' + varname + '.h\n')
