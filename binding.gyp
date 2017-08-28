
{
    'variables': {
    },
    'targets': [
        {
            'target_name': 'heif',
            'dependencies': [
                'src/deps/heif/heiflib.gyp:'
            ],
            'cflags': [
                
            ],
            'defines': [
                
            ],
            'include_dirs': [
                "<!(node -e \"require('nan')\")"
            ],
            'sources': [
                'src/heif_reader.cc',
                'src/heif_writer.cc'
            ],
            'link_settings': {
                'ldflags': [
                ],
                'libraries': [
                ]
            }
        }
    ]
}