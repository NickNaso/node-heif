
{
    'variables': {
    },
    'targets': [
        {
            'target_name': 'Heif',
            #'dependencies': [
            #    'src/deps/heif/heiflib.gyp:'
            #],
            'cflags': [
                
            ],
            'defines': [
                
            ],
            'include_dirs': [
                "<!(node -e \"require('nan')\")"
            ],
            'sources': [
                'src/heif.cc'
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