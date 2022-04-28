# import("env")

# # access to global build environment
# env.Execute(
#     "C:\\Progra~1\\Git\\mingw64\\bin\\openssl.exe genrsa -out private.key 2048")
# env.Execute("C:\\Progra~1\\Git\\mingw64\\bin\\openssl.exe rsa -in private.key -outform PEM -pubout -out public.key")
import sys
from cryptography.hazmat.primitives import serialization as crypto_serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.backends import default_backend as crypto_default_backend

key = rsa.generate_private_key(
    backend=crypto_default_backend(),
    public_exponent=65537,
    key_size=2048
)

private_key = key.private_bytes(
    crypto_serialization.Encoding.PEM,
    crypto_serialization.PrivateFormat.PKCS8,
    crypto_serialization.NoEncryption()
)
with open('./private.key', "wb") as f:
    f.write(private_key)
    sys.stderr.write('Wrote ./private.key\n')

public_key = key.public_key().public_bytes(
    crypto_serialization.Encoding.PEM,
    crypto_serialization.PublicFormat.PKCS1
)
with open('./public.key', "wb") as f:
    f.write(public_key)
    sys.stderr.write('Wrote ./public.key\n')

val = 'const char signing_pubkey[] PROGMEM = {\n '
j = 0
for i in bytearray(public_key):
    val += " 0x%02x," % i
    j = j + 1
    if j == 63:
        val += '\n '
        j = 0
if j == 0:
    val = val[:-2]  # prune off the newline,
val = val[:-1]  # prune off the comma
val += "\n};\n"
with open('./include/public.key.h', "w") as f:
    f.write(val)
    sys.stderr.write('Wrote ./include/public.key.h\n')
