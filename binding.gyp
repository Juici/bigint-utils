{
    "targets": [
        {
            "target_name": "bigint_utils",
            "sources": ["src/node/native/module.c"],
            "configurations": {
                "Release": {
                    "defines": ["NDEBUG"],
                }
            },
        }
    ],
}
