#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conan.packager import ConanMultiPackager
from conans.util.env_reader import get_env
import os

if __name__ == "__main__":
    librw_platform = get_env("librw_platform", "unknown")
    if librw_platform == "unknown":
        raise RuntimeError("Need librw_platform environment variable")

    builder = ConanMultiPackager()
    builder.add_common_builds()

    for b in builder.builds:
        b[1]['platform'] = librw_platform
        print(b)

    builder.run()
