{
    "targets": [
        {
            "target_name": "bigint_utils",
            "sources": ["src/node/native.c"],
            "configurations": {
                "Release": {
                    "defines": ["NDEBUG"],
                }
            },
        }
    ],
}
