import sys
import subprocess
import gzip

Import("env")
# Yonneh@AMDR9 MINGW64 /c/Projects/esp32/stepper-8266/.pio/build/esp12e (main)
# $ gzip -9 -k firmware.bin


def after_build():
    with open('./.pio/build/esp12e/firmware.bin', 'rb') as b:
        inp = b.read()

    data = gzip.compress(inp)

    with open('./.pio/build/esp12e/firmware.bin.gz', mode='wb') as g:
        g.write(data)

    signcmd = ['C:\\Progra~1\\Git\\mingw64\\bin\\openssl.exe',
               'dgst', '-sha256', '-sign', "./private.key"]
    proc = subprocess.Popen(signcmd, stdout=subprocess.PIPE,
                            stdin=subprocess.PIPE, stderr=subprocess.PIPE)
    signout, signerr = proc.communicate(input=data)
    if proc.returncode:
        sys.stderr.write("OpenSSL returned an error signing the binary: " +
                         str(proc.returncode) + "\nSTDERR: " + str(signerr))
    else:
        with open("./signed_firmware.bin.gz", "wb") as out:
            out.write(data)
            out.write(signout)
            out.write(b'\x00\x01\x00\x00')
            sys.stderr.write("Signed binary: " +
                             "./signed_firmware.bin.gz" + "\n")


# Doesn't actually work- just build twice.
env.AddPostAction("buildprog", after_build)
try:
    after_build()
except:
    f = 2  # how do you do nothing in python? who picked python for this?

with open('./public.key', "rb") as f:
    pub = f.read()

val = 'const char signing_pubkey[] PROGMEM = {\n '
j = 0
for i in bytearray(pub):
    val += " 0x%02x," % i
    j = j + 1
    if j == 127:
        val += '\n '
        j = 0
if j == 0:
    val = val[:-2]  # prune off the newline,
val = val[:-1]  # prune off the comma
val += "\n};\n"
sys.stderr.write("Enabling binary signing\n")
with open('./include/public.key.h', "w") as f:
    f.write(val)
    sys.stderr.write('Rebuilt ./public.key as ./include/public.key.h\n')

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
