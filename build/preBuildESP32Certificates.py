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

import argparse
import csv
import os
import re
import struct
import sys
from io import open

def preBuildCertificates(env):
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

    ca_bundle_bin_file = 'x509_crt_bundle.bin'
    quiet = False

    class CertificateBundle:
        def __init__(self):
            self.certificates = []
            self.compressed_crts = []

            # TODO: reuse cached file
            if os.path.isfile(ca_bundle_bin_file):
                os.remove(ca_bundle_bin_file)
    
        def add_from_file(self, file_path):
            try:
                print('Parsing certificates from %s' % file_path)
                with open(file_path, 'r', encoding='utf-8') as f:
                    self.add_from_pem(f.read())
                    return True

            except ValueError:
                raise InputError('Invalid certificate in %s' % file_path)

            return False

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

            if(count == 0):
                raise InputError('No certificate found')

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

    class InputError(RuntimeError):
        def __init__(self, e):
            super(InputError, self).__init__(e)

    bundle = CertificateBundle()
    bundle.add_from_file("build/ca-bundle.crt")
    print('Successfully added %d certificates in total' % len(bundle.certificates))

    crt_bundle = bundle.create_bundle()

    with open(ca_bundle_bin_file, 'wb') as f:
        f.write(crt_bundle)


if __name__ == '__main__':
    try:
        preBuildCertificates(None)
    except InputError as e:
        print(e)
        sys.exit(2)