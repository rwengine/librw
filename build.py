#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conan.packager import ConanMultiPackager
from conans.util.env_reader import get_env
from conans import tools

if __name__ == "__main__":
    librw_platform = get_env("librw_platform", "unknown")
    if librw_platform == "unknown":
        raise RuntimeError("Need librw_platform environment variable")

    with tools.environment_append({
        "CONAN_REMOTES": "https://api.bintray.com/conan/bincrafters/public-conan@True@bincrafters",
    }):
        builder = ConanMultiPackager()
        builder.add(
            options={"platform": librw_platform, }
        )

        # builder.add_common_builds()
        # for b in builder.builds:
        #     b[1]['platform'] = librw_platform
        #     print(b)

        builder.run()
