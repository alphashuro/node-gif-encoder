{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ 
        "addon.cc",
        "src/node-wrapper.cpp",
        "src/gif-encoder.cpp",
        "src/typed-neu-quant.cpp",
        "src/lzw-encoder.cpp",
        "src/byte-array.cpp"
      ],
      'libraries': ['-framework OpenGL', '-framework OpenCL'],
      "include_dirs": [
        "include",
        "/usr/local/include/boost"
      ],
      'library_dirs': ['/usr/local/lib'],
      "cflags_cc!": [ "-fno-rtti", "-fno-exceptions" ],
      "cflags!": [ "-fno-exceptions", "-stdlib=libc++", "-std=c++17" ],
      "conditions": [
        ['OS=="mac"', {
          'make_global_settings': [
            ['CC', '/usr/bin/clang'],
            ['CXX', '/usr/bin/clang++'],
          ],
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