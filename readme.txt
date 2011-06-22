spraymaker

= WTF =
easy to create VTF file(spray) for Valve SourceEngine Games.
if image file drag & drop to spraymaker.exe then create high quality spray file.

= Description =
High quality spray maker for valve source engine.
 * 256 x 256 size spray
 * output alpha channel VTF(BGRA8888 = high quality compression type)
 * add high quality VTF option (NOMAP, NOLOD)
 * read can 200 over image types
 * keep original aspect ratio (auto padding transparent pixels)
 * read can directory (recursive convertion)

more description is here
http://kimoto.github.com/spraymaker.html

= Compile =
#1. setup dependencies files
this program dependencies ImageMagick++, VTFLib library
download from official website and extract following hierarchy
------------------------------
./spraymaker/VTFLib.dll
./spraymaker/vtflib131-bin/
./spraymaker/magick/include/
./spraymaker/magick/lib/
------------------------------

auto download & setting script exist. recommend use this.
written by Ruby programming language.
------------------------------
$ ruby ./download_libs.rb
------------------------------

