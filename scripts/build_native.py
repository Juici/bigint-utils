#!/usr/bin/env python3

import argparse
import os
import shlex
import shutil
import subprocess
import sys

from typing import List, Optional


PROJECT_NAME = "bigint-utils"
LIB_NAME = PROJECT_NAME.replace("-", "_")


class ParsedArgs(argparse.Namespace):
    out_dir: Optional[str]
    build_dir: str
    profile: str
    os: str
    arch: str
    target: str


def joinSpace(args: List[str]) -> str:
    return " ".join(args)


def main() -> int:
    args = sys.argv[1:]
    if sys.platform == "win32":
        args = shlex.split(joinSpace(args), posix=False)
    print(f"Invoked with '{joinSpace(args)}'")

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--out-dir",
        default=None,
        metavar="DIR",
        help="specify destination dir (default: build/$CONFIGURATION_NAME)",
    )
    parser.add_argument(
        "--build-dir",
        default="target",
        metavar="DIR",
        help="specify cargo build dir (default: %(default)s)",
    )
    parser.add_argument(
        "--profile",
        default="Release",
        choices=["Release", "Debug"],
        metavar="PROFILE",
        help="specify build profile (%(choices)s)",
    )
    parser.add_argument(
        "--os",
        required=True,
        metavar="OS",
        help="specify node os name",
    )
    parser.add_argument(
        "--arch",
        required=True,
        metavar="ARCH",
        help="specify node arch (eg. x64, ia32, arm64)",
    )
    parser.add_argument(
        "--target",
        required=True,
        metavar="TARGET",
        help="specify cargo target",
    )

    opts = parser.parse_args(args, namespace=ParsedArgs())

    profile = opts.profile
    out_dir = opts.out_dir or os.path.join("build", profile)
    build_dir = opts.build_dir
    node_os_name = opts.os
    node_arch = opts.arch
    cargo_target = opts.target

    cmdline = ["cargo", "build", "--target", cargo_target, "-p", PROJECT_NAME]
    if profile == "Release":
        cmdline.append("--release")
    print(f"Running '{joinSpace(cmdline)}'")

    cargo_env = os.environ.copy()
    cargo_env["CARGO_BUILD_TARGET_DIR"] = build_dir
    # On Linux, cdylibs don't include public symbols from their dependencies,
    # even if those symbols have been re-exported in the Rust source.
    # Using LTO works around this at the cost of a slightly slower build.
    # https://github.com/rust-lang/rfcs/issues/2771
    cargo_env["CARGO_PROFILE_RELEASE_LTO"] = "thin"

    if node_os_name == "win32":
        # By default, Rust on Windows depends on an MSVC component for the C runtime.
        # Link it statically to avoid propagating that dependency.
        cargo_env["RUSTFLAGS"] = "-C target-feature=+crt-static"

    cmd = subprocess.Popen(cmdline, env=cargo_env)
    if cmd.wait() != 0:
        print("error: cargo build failed")
        return 1

    libs_dir = os.path.join(build_dir, cargo_target, profile.lower())

    for lib_format in ["{}.dll", "lib{}.so", "lib{}.dylib"]:
        src_path = os.path.join(libs_dir, lib_format.format(LIB_NAME))
        if os.access(src_path, os.R_OK):
            dst_path = os.path.join(out_dir, f"{LIB_NAME}_{node_os_name}_{node_arch}.node")
            print(f"Copying {src_path} to {dst_path}")
            if not os.path.exists(out_dir):
                os.makedirs(out_dir)
            shutil.copyfile(src_path, dst_path)
            return 0

    print("error: could not find generated library")
    return 1


if __name__ == "__main__":
    sys.exit(main())
