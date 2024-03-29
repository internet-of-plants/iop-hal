#!/usr/bin/env python
#
# ESP32 x509 certificate bundle generation utility
#
# Converts PEM and DER certificates to a custom bundle format which stores just the
# subject name and public key to reduce space
#
# The bundle will have the format: number of certificates; crt 1 subject name length; crt 1 public key length;
# crt 1 subject name; crt 1 public key; crt 2...
#
# Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import with_statement

import time
from os import path
import inspect
import subprocess
import os
import struct
import hashlib
from io import open

try:
    from urllib.request import urlopen
except Exception:
    from urllib2 import urlopen

def preBuildCertificates(env):
    dir_path = path.dirname(path.abspath(inspect.getframeinfo(inspect.currentframe()).filename))
    destination = path.join(dir_path, "../src/arduino/esp/esp32/generated/certificates.hpp")
    cache = path.join(dir_path, "./certdata.txt")

    # Avoid downloading if certificates were generated less than 7 days ago
    try:
        if time.time() - path.getmtime(destination) < 3600 * 24 * 7:
            return
    except OSError:
        pass

    try:
        response = urlopen('https://hg.mozilla.org/releases/mozilla-release/raw-file/default/security/nss/lib/ckfw/builtins/certdata.txt')
        hash = hashlib.sha256(response.read()).hexdigest()
    except Exception:
        if path.exists(destination):
            print("Unable to fetch certificates from the network, using cached certificates")
        else:
            raise Exception("Unable to fetch certificates from the network, and no certificates are available locally")
        return

    try:
        with open(destination, "r") as generated:
            for line in filter(lambda x: "SHA256: " in x, generated.read().split("\n")):
                if line.split("SHA256: ")[1] == hash:
                    print("Certificates already are up-to-date")
                    return
                break
    except FileNotFoundError:
        pass

    try:
        from cryptography import x509
        from cryptography.hazmat.backends import default_backend
        from cryptography.hazmat.primitives import serialization
    except ImportError:
        if env is None:
            raise Exception("Must install cryptography library before running")
        env.Execute("$PYTHONEXE -m pip install cryptography==36.0.2")
        try:
            from cryptography import x509
            from cryptography.hazmat.backends import default_backend
            from cryptography.hazmat.primitives import serialization
        except Exception:
            if path.exists(destination):
                print("Unable to fetch certificates from the network, using cached certificates")
            else:
                raise Exception("Unable to fetch certificates from the network, and no certificates are available locally")
            return

    class CertificateBundle:
        def __init__(self):
            self.certificates = []
            self.compressed_crts = []

            try:
                os.remove(destination)
            except Exception:
                pass

        def add_from_file(self, file_path):
            try:
                print('Parsing certificates from %s' % file_path)
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.add_from_pem(f.read())
                    return True

            except ValueError:
                raise Exception('Invalid certificate in %s' % file_path)

        def add_from_pem(self, crt_str):
            """ A single PEM file may have multiple certificates """

            crt = ''
            count = 0
            start = False

            for strg in crt_str.splitlines(True):
                if strg == '-----BEGIN CERTIFICATE-----\n' and start is False:
                    crt = ''
                    start = True
                elif strg == '-----END CERTIFICATE-----\n' and start is True:
                    crt += strg + '\n'
                    start = False
                    self.certificates.append(x509.load_pem_x509_certificate(crt.encode(), default_backend()))
                    count += 1
                if start is True:
                    crt += strg

            if count == 0:
                raise Exception('No certificate found')

            print('Successfully added %d certificates' % count)

        def create_bundle(self):
            # Sort certificates in order to do binary search when looking up certificates
            self.certificates = sorted(self.certificates, key=lambda cert: cert.subject.public_bytes(default_backend()))

            bundle = struct.pack('>H', len(self.certificates))

            for crt in self.certificates:
                """ Read the public key as DER format """
                pub_key = crt.public_key()
                pub_key_der = pub_key.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)

                """ Read the subject name as DER format """
                sub_name_der = crt.subject.public_bytes(default_backend())

                name_len = len(sub_name_der)
                key_len = len(pub_key_der)
                len_data = struct.pack('>HH', name_len, key_len)

                bundle += len_data
                bundle += sub_name_der
                bundle += pub_key_der

            return bundle

    print("Downloading openssl certificates")

    # Avoid running perl script if it was last run less than 7 days ago
    try:
        has_cache = time.time() - path.getmtime(cache) < 3600 * 24 * 7
    except OSError:
        has_cache = False

    if not has_cache:
        subprocess.call(["perl", "mk-ca-bundle.pl"])

    print("Successfully downloaded openssl certificates")

    bundle = CertificateBundle()
    bundle.add_from_file("ca-bundle.crt")
    print('Added %d certificates in total' % len(bundle.certificates))

    crt_bundle = bundle.create_bundle()

    with open(destination, 'w') as f:
        f.write("#ifndef IOP_ESP32_CERTIFICATES_H\n")
        f.write("#define IOP_ESP32_CERTIFICATES_H\n")
        f.write("#ifdef IOP_ESP32\n")
        f.write("namespace generated {\n")
        f.write("// This file is computer generated at build time (`build/preBuildESP32Certificates.py` called by PlatformIO)\n\n")
        f.write("// SHA256: " + str(hash) + "\n\n")
        f.write("static const uint8_t certs_bundle[] IOP_ROM = {")
        f.write(",".join([str(int(b)) for b in crt_bundle]))
        f.write("};\n\n")
        f.write("} // namespace generated\n")
        f.write("\n#endif" + "\n")
        f.write("\n#endif" + "\n")

if __name__ == '__main__':
    preBuildCertificates(None)