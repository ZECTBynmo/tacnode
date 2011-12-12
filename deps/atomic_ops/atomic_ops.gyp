# This file is used with the GYP meta build system.
# http://code.google.com/p/gyp/
# To build try this:
#   svn co http://gyp.googlecode.com/svn/trunk gyp
#   ./gyp/gyp -f make --depth=`pwd` atomic_ops.gyp
#   ./out/Debug/test 
{
  'targets': [
    {
      'target_name': 'atomic_ops',
      'type': 'static_library',
      'include_dirs': [ 'src' ],
      'direct_dependent_settings': {
        'include_dirs': [ 'src' ],
      },
    },
  ]
}
