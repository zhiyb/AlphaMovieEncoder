@rem lib /def:amenc.def /out:amenc.lib /MACHINE:x86
cl /O2 /EHsc /std:c++latest amenc.cpp /Feamenc.exe
@rem /link amenc.lib
amenc.exe
