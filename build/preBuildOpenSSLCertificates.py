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

import time
from os import path
import inspect
import subprocess
import hashlib
from io import open

try:
    from urllib.request import urlopen
except Exception:
    from urllib2 import urlopen

def preBuildCertificates(env):
    dir_path = path.dirname(path.abspath(inspect.getframeinfo(inspect.currentframe()).filename))
    destination = path.join(dir_path, "../src/openssl/generated/certificates.hpp")
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

    print("Downloading openssl certificates")

    # Avoid running perl script if it was last run less than 7 days ago
    try:
        has_cache = time.time() - path.getmtime(cache) < 3600 * 24 * 7
    except OSError:
        has_cache = False

    if not has_cache:
        subprocess.call(["perl", "mk-ca-bundle.pl"])
    print("Successfully downloaded openssl certificates")

    with open(path.join(dir_path, "ca-bundle.crt"), "r") as ca:
        certs = ca.read()

        with open(destination, "w") as f:
            f.write("#ifndef IOP_OPENSSL_CERTIFICATES_H\n")
            f.write("#define IOP_OPENSSL_CERTIFICATES_H\n")
            f.write("#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)\n")
            f.write("namespace generated {\n")
            f.write("// This file is computer generated at build time (`build/preBuildOpenSSLCertificates.py` called by PlatformIO)\n\n")
            f.write("// SHA256: " + str(hash) + "\n\n")
            f.write("static const char certs_bundle[] IOP_ROM = \"\"\\\n\"")
            f.write(certs.replace("\n", "\\n\"\\\n\""))
            f.write("\";\n\n")
            f.write("} // namespace generated\n")
            f.write("#endif" + "\n")
            f.write("#endif" + "\n")

if __name__ == "__main__":
    preBuildCertificates(None)