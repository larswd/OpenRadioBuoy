# OLB firmware
The firmware for the OLB is different applications bulit around a set of sometimes shared libraries. In fact, each application folder, currently ```drifter``` and ```basestation``` each only consist of an unique ```main.cpp``` and ```config.h```. All modules that relate to one or more component is located in the ```common_libraries``` folder, and is included in the applications through the ```platformio.ini``` file. The current modules are, with descriptions and dependencies (with the exception of the excellent [Embedded Template Library (ETL)](https://www.etlcpp.com/) which is a dependency for all the modules more or less)

| Module | Description | Dependencies |
| ------ | ----------- | ------------ |
| [```etl_error_manager```](common_libraries/etl_error_manager/) | Here we specify how ETL is supposed to handle different errors | None |
| ```gps_manager``` | Handling of the onboard GPS and its measurements | [```etl_error_manager```](common_libraries/etl_error_manager/), ```thermo_manager```, ```stats```, ```sd_writer``` [```TinyGPSPlus```](https://github.com/mikalhart/TinyGPSPlus), ```parser_utils```  |
| ```sd_writer``` | All operations when it comes to file management and file writing to the onboard sd card | ```etl_error_manager``` |