#!/usr/bin/env python3

from __future__ import print_function
from os import path, unlink
import time
import inspect
import csv
import re
import string
import sys
import hashlib
from shutil import which

# Based on https://github.com/arduino/esp8266/blob/master/libraries/ESP8266WiFi/examples/BearSSL_CertStore/certs-from-mozilla.py
# Adapted to be able to bake the certificates into the source code

try:
    from urllib.request import urlopen
except Exception:
    from urllib2 import urlopen
try:
    from StringIO import StringIO
except Exception:
    from io import StringIO

from subprocess import Popen, PIPE

def preBuildCertificates(env):
    dir_path = path.dirname(path.abspath(inspect.getframeinfo(inspect.currentframe()).filename))
    destination = path.join(dir_path, "../src/arduino/esp/esp8266/generated/certificates.hpp")

    # Avoid downloading if certificates were generated less than 7 days ago
    try:
        if time.time() - path.getmtime(destination) < 3600 * 24 * 7:
            return
    except OSError:
        pass

    try:
        from asn1crypto.x509 import Certificate
    except ImportError:
        if env is None:
            raise Exception("Must install asn1crypto library before running")
        env.Execute("$PYTHONEXE -m pip install asn1crypto==1.4.0")
        try:
            from asn1crypto.x509 import Certificate
        except Exception:
            if path.exists(destination):
                print("Unable to find or install asn1crypto python library, using cached certificates")
            else:
                raise Exception("Unable to find or install asn1crypto python library, and no certificates are available locally")

    # check if openssl is available
    if which('openssl') is None and not path.isfile('./openssl') and not path.isfile('./openssl.exe'):
        raise Exception("You need to have openssl in PATH, installable from https://www.openssl.org/")

    # Mozilla's URL for the CSV file with included PEM certs
    mozurl = "https://ccadb-public.secure.force.com/mozilla/IncludedCACertificateReportPEMCSV"

    # Load the names[] and pems[] array from the URL
    names = []
    pems = []
    try:
        response = urlopen(mozurl)
    except Exception:
        if path.exists(destination):
            print("Connection failed, using cached certificates")
        else:
            raise Exception("Connection failed, unable to get certificates and no cached version is available")
    csvData = response.read()
    csvHash = hashlib.sha256(csvData).hexdigest()

    try:
        with open(destination, "r") as generated:
            current = generated.read().split("\n")
            for line in filter(lambda x: "SHA256: " in x, current):
                if line.split("SHA256: ")[1] == csvHash:
                    print("Certificates already are up-to-date")
                    return
                break
    except FileNotFoundError:
        pass

    if sys.version_info[0] > 2:
        csvData = csvData.decode('utf-8')
    csvFile = StringIO(csvData)
    csvReader = csv.reader(csvFile)
    for row in csvReader:
        names.append(row[0]+":"+row[1]+":"+row[2])
        for item in row:
            if item.startswith("'-----BEGIN CERTIFICATE-----"):
                pems.append(item)
    del names[0]
    del pems[0]

    print("Generating " + destination)

    f = open(destination, "w", encoding="utf8")

    f.write("#ifndef IOP_ESP8266_CERTIFICATES_H\n")
    f.write("#define IOP_ESP8266_CERTIFICATES_H\n")
    f.write("#ifdef IOP_ESP8266\n")
    f.write("#include \"arduino/esp/esp8266/cert_store.hpp\"\n\n")
    f.write("namespace generated {\n")
    f.write("// This file is computer generated at build time (`build/preBuildESP8266Certificates.py` called by PlatformIO)\n\n")
    f.write("// SHA256: " + str(csvHash) + "\n\n")

    certFiles = []
    totalbytes = 0
    idx = 0

    # Process the text PEM using openssl into DER files
    sizes=[]
    for i in range(0, len(pems)):
        certName = "ca_%03d" % (idx);
        thisPem = pems[i].replace("'", "")

        with open(certName + '.pem', "w", encoding="utf8") as pemfile:
            pemfile.write(thisPem)

        f.write(("// " + re.sub(f'[^{re.escape(string.printable)}]', '', names[i]) + "\n"))

        with Popen(['openssl','x509','-inform','PEM','-outform','DER','-out', certName + '.der'], shell = False, stdin = PIPE) as ssl:
            ssl.stdin.write(thisPem.encode('utf-8'))
            ssl.stdin.close()
            ssl.wait()

        if path.exists(certName + '.der'):
            certFiles.append(certName)

            with open(certName + '.der','rb') as der:
                bytestr = der.read();
                sizes.append(len(bytestr))
                cert = Certificate.load(bytestr)
                idxHash = hashlib.sha256(cert.issuer.dump()).digest()

                f.write("static const uint8_t cert_" + str(idx) + "[] IOP_ROM = {")
                for j in range(0, len(bytestr)):
                    totalbytes+=1
                    f.write(hex(bytestr[j]))
                    if j<len(bytestr)-1:
                        f.write(", ")
                f.write("};\n")

                f.write("static const uint8_t idx_" + str(idx) + "[] IOP_ROM = {")
                for j in range(0, len(idxHash)):
                    totalbytes+=1
                    f.write(hex(idxHash[j]))
                    if j<len(idxHash)-1:
                        f.write(", ")
                f.write("};\n\n")

            idx = idx + 1
            unlink(certName + '.der')
        unlink(certName + '.pem')

    f.write("static const uint16_t numberOfCertificates = " + str(idx) + ";\n\n")

    f.write("static const uint16_t certSizes[] IOP_ROM = {")
    for i in range(0, idx):
        f.write(str(sizes[i]))
        if i<idx-1:
            f.write(", ")
    f.write("};\n\n")

    f.write("static const uint8_t* const certificates[] IOP_ROM = {")
    for i in range(0, idx):
        f.write("cert_" + str(i))
        if i<idx-1:
            f.write(", ")
    f.write("};\n\n")

    f.write("static const uint8_t* const indexes[] IOP_ROM = {")
    for i in range(0, idx):
        f.write("idx_" + str(i))
        if i<idx-1:
            f.write(", ")
    f.write("};\n\n")
    f.write("static const iop_hal::CertList certList(certificates, indexes, certSizes, numberOfCertificates);\n")
    f.write("} // namespace generated\n")
    f.write("\n#endif" + "\n")
    f.write("\n#endif" + "\n")

    f.close()

if __name__ == "__main__":
    preBuildCertificates(None)
