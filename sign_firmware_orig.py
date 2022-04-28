import sys
import subprocess
import gzip

with open('./.pio/build/esp12e/firmware.bin', 'rb') as b:
    inp = b.read()

data = gzip.compress(inp)

with open('./firmware.bin.gz', mode='wb') as g:
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
