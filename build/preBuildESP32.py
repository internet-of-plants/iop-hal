#!/usr/bin/env python3

Import('env')
from preBuildESP32Certificates import preBuildCertificates

preBuildCertificates(env)