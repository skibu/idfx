# Makefile for idfx componennt.
# Need to specify what is to be compiled in SRC_DIRS, the usual INCLUDE_DIRS 
# of "include", and importantly, all dependencies in the REQUIRES file. If
# don't specify the depenedencies then the compiler won't even know where
# to find the include files.
idf_component_register(SRC_DIRS "src/utils" "src/hardware" "src/display"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp-idf-cxx" "esp_driver_ledc" "lvgl")
