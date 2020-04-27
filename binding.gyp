{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ 
        "addon.cc",
        "gif-encoder.cpp",
        "lzw-encoder.cpp",
        "typed-neu-quant.cpp",
        "byte.cpp",
        "byte-array.cpp"
      ],
      "include_dirs": [
        "/usr/local/include",
        "/usr/local/include/boost"
      ],
      "libraries": [
          # "/usr/local/lib/libboost_regex.dylib"
      ],
      "cflags_cc!": [ "-fno-rtti", "-fno-exceptions" ],
      "cflags!": [ "-fno-exceptions", "-stdlib=libc++", "-std=c++17" ],
      "conditions": [
        ['OS=="mac"', {
            "xcode_settings": {
                'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++', '-v'],
                'OTHER_LDFLAGS': ['-stdlib=libc++'],
                'MACOSX_DEPLOYMENT_TARGET': '10.7',
                  'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
              }
        }]
      ]
    }
  ]
}