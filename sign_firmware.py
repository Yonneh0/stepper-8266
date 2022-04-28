import sys
# import subprocess
import gzip
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.serialization import (
    load_pem_private_key
)

with open('./private.key', 'rb') as p:
    private_key = p.read()
    private_key_object = load_pem_private_key(
        private_key, password=None, backend=default_backend())

with open('./.pio/build/esp12e/firmware.bin', 'rb') as b:
    firmware_bin = b.read()

firmware_bin_gz = gzip.compress(firmware_bin)

with open('./firmware.bin.gz', mode='wb') as g:
    g.write(firmware_bin_gz)

signature = private_key_object.sign(
    firmware_bin_gz, padding.PKCS1v15(), hashes.SHA256())

with open("./signed_firmware.bin.gz", "wb") as out:
    out.write(firmware_bin_gz)
    out.write(signature)
    out.write(b'\x00\x01\x00\x00')
    sys.stderr.write("Signed binary: " +
                     "./signed_firmware.bin.gz" + "\n")

    # with open('./.pio/build/esp12e/firmware.bin', 'rb') as b:
    #     inp = b.read()

    # data = gzip.compress(inp)

    # with open('./.pio/build/esp12e/firmware.bin.gz', mode='wb') as g:
    #     g.write(data)

    # signcmd = ['C:\\Progra~1\\Git\\mingw64\\bin\\openssl.exe',
    #            'dgst', '-sha256', '-sign', "./private.key"]
    # proc = subprocess.Popen(signcmd, stdout=subprocess.PIPE,
    #                         stdin=subprocess.PIPE, stderr=subprocess.PIPE)
    # signout, signerr = proc.communicate(input=data)
    # if proc.returncode:
    #     sys.stderr.write("OpenSSL returned an error signing the binary: " +
    #                      str(proc.returncode) + "\nSTDERR: " + str(signerr))
    # else:
    #     with open("./signed_firmware.bin.gz", "wb") as out:
    #         out.write(data)
    #         out.write(signout)
    #         out.write(b'\x00\x01\x00\x00')
    #         sys.stderr.write("Signed binary: " +
    #                          "./signed_firmware.bin.gz" + "\n")
