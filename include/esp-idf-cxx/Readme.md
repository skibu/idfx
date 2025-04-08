# esp-idf-cxx include files

This directory contains include files from the Espressif component
esp-idf-cxx. This component is Espressif's attempt at providing C++
wrappers on their C code. This code has some useful classes, but
has some serious flaws, such as only using v1 of the I2C driver, which
is going to be soon obsoleted. 

Specifically, with esp-idf-cxx the file i2c_cxx.cpp 
was getting compiled and it includes "driver/i2c.h" which causes the
old v1 I2C driver to wrongly be loaded. To get around this need
to not have esp-idf-cxx be a dependency, but still make the .hpp
files in this directory available. 

Therefore to make available the useful classes some of the .hpp files
have been duplicated here. The only changes to the .hpp files are 1) that
the include directives have been updated to point to this directory and 2)
that namespace was changed from idf to idfx.