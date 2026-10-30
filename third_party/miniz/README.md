# miniz inflate component

miniz 3.0.2, https://github.com/richgel999/miniz/tree/3.0.2
Copyright and MIT terms: LICENSE in this directory.

Only the portable standalone tinfl decompressor and its headers are bundled.
Kleiotide uses it to read DEFLATE-compressed XLSX ZIP entries. There is no added
runtime DLL or separately installed build dependency. The source is compiled as
C++; its include was changed from miniz.h to miniz_tinfl.h to build only this
component. miniz_export.h defines MINIZ_EXPORT as empty for static inclusion.
The upstream license is installed beside the application in third-party/miniz.
