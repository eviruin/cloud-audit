{
  "targets": [
    {
      "target_name": "cloud_breaker",
      "sources": [ "binding.cpp" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [],
      "conditions": [
        ["OS=='linux'", {
          "cflags": [ "-std=c++11" ],
          "libraries": []
        }]
      ]
    }
  ]
}