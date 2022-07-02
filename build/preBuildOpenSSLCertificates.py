#!/usr/bin/env python
#
# x509 certificate bundle generation utility
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

from os import path
import inspect
import subprocess
import os
import struct
import hashlib
import sys
from io import open

folder = "../src/openssl/generated/"
destination = path.join(folder, "certificates.hpp")

class InputError(RuntimeError):
    def __init__(self, e):
        super(InputError, self).__init__(e)

def preBuildCertificates(env):
    filename = inspect.getframeinfo(inspect.currentframe()).filename
    dir_path = path.dirname(path.abspath(filename))

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
            raise Exception("Unable to find or install cryptography python library, and no certificates are available locally")

    p = subprocess.run(["perl", "mk-ca-bundle.pl"])

    print("Successfully downloaded openssl certificates")

    with open(path.join(dir_path, "ca-bundle.crt"), "r") as ca:
        certs = ca.read()
        hash = hashlib.sha256(certs.encode("utf-8")).hexdigest()

    with open(path.join(dir_path, destination), "w") as f:
        f.write("#ifndef IOP_PEM_CERTIFICATES_H\n")
        f.write("#define IOP_PEM_CERTIFICATES_H\n")
        f.write("#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)\n")
        f.write("namespace generated {\n")
        f.write("// This file is computer generated at build time (`build/preBuildPEMCertificates.py` called by PlatformIO)\n\n")
        f.write("// SHA256: " + str(hash) + "\n\n")
        f.write("static const char certs_bundle[] IOP_ROM = \"\"\\\n\"")
        f.write(certs.replace("\n", "\\n\"\\\n\""))
        f.write("\";\n\n")
        f.write("} // namespace generated\n")
        f.write("\n#endif" + "\n")
        f.write("\n#endif" + "\n")

if __name__ == "__main__":
    try:
        preBuildCertificates(None)
    except InputError as e:
        print(e)
        sys.exit(2)