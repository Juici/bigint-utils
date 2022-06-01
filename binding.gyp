{
    "conditions": [
        [
            "OS=='mac'",
            {
                "variables": {
                    "NODE_OS_NAME": "darwin",
                    "CARGO_TARGET_SUFFIX": "apple-darwin",
                }
            },
            "OS=='win'",
            {
                "variables": {
                    "NODE_OS_NAME": "win32",
                    "CARGO_TARGET_SUFFIX": "pc-windows-msvc",
                }
            },
            "OS=='linux'",
            {
                "variables": {
                    "NODE_OS_NAME": "linux",
                    "CARGO_TARGET_SUFFIX": "unknown-linux-gnu",
                }
            },
            {"variables": {"NODE_OS_NAME": "<(OS)"}},
        ],
        [
            "target_arch=='ia32'",
            {"variables": {"CARGO_ARCH": "i686"}},
            "target_arch=='x64'",
            {"variables": {"CARGO_ARCH": "x86_64"}},
            "target_arch=='arm64'",
            {"variables": {"CARGO_ARCH": "aarch64"}},
        ],
    ],
    "targets": [
        {
            "target_name": "bigint_utils_<(NODE_OS_NAME)_<(target_arch).node",
            "type": "none",
            "actions": [
                {
                    "action_name": "build_native.py",
                    "action": [
                        "python3",
                        "scripts/build_native.py",
                        # Use separated arguments for paths, joined arguments for non-paths.
                        "--out-dir",
                        "<(PRODUCT_DIR)/",
                        "--build-dir",
                        "<(INTERMEDIATE_DIR)/rust",
                        "--profile=<(CONFIGURATION_NAME)",
                        "--os=<(NODE_OS_NAME)",
                        "--arch=<(target_arch)",
                        "--target=<(CARGO_ARCH)-<(CARGO_TARGET_SUFFIX)",
                    ],
                    "inputs": [],
                    "outputs": [
                        "<(PRODUCT_DIR)/<(_target_name)",
                        # This really needs to be environment-variable-sensitive, but node-gyp doesn't support that. Cargo will still save work if possible.
                        "<(PRODUCT_DIR)/nonexistent-file-to-force-rebuild",
                    ],
                }
            ],
        }
    ],
}
