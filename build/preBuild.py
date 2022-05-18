#!/usr/bin/env python3

Import('env')

from preBuildESP8266Certificates import preBuildCertificates

preBuildCertificates(env)

from preBuildESP32Certificates import preBuildCertificates

preBuildCertificates(env)