# libid666

A small c library for parsing an SPC file
and extracting metadata - known as id666
tags.

Methods and details in the `id666.h` file.

# Building

Just run `make`. By default, builds:

* `libid666.a`
* `libid666.so`

You can specify `STATIC_PREFIX` and `DYNLIB_PREFIX` if you
want/need to change/remove `lib` from the filenames.

You can specify `STATIC_EXT` and `DYNLIB_EXT` if you
want/need to change the extensions from `.a` and `.so`
to something else.

`make install` will install to `/usr/local`, you can specify
`PREFIX=` to change the prefix, and `DESTDIR=` to set a
staging directory.

There's two optional programs you can build:

* `id666-dump` - just dumps tag contents
* `id666-gme-test` - links against libgme, checks if
both libraries parse a given SPC the same way.

# License

Unless otherwise stated, all files are released under
an MIT-style license. Details in `LICENSE`

Some exceptions:

* `attr.h ` - public domain, details found in file

