# The OLB code - a short primer
The OLB code is extensive, and reading the code of others is always difficult. Hence, we have created this short document explaining the code and how to use it properly. This file is structured in two parts. First, we have a general overview over each module which together constitute the OLB, then we have a short primer on how to write your own module to seamlessly integrate it into the OLB source code, and last we have a guide on how to use the config file to tweak most buoy settings without having to code anything yourself. 

## The OLB source code in a shellnut 
The general logic of the OLB is sentered around its LoRa library, which implements a class called ```LoRa_Transceiver``` and one default instance of said class with the name ```LORA```. Most other classes are designed around being integrated with the ```LoRa_Transceiver``` class. This class is initialized once during setup, where it transmits its deployment position, its ID, and the date of deployment. The buoy ID is constructed from the serial number of the thermometre. To use the ```LoRa_Transceiver``` module, call the ```beginRadio(LoRa_freq_send, LoRa_bw, LoRa_sf, LoRa_cr, LoRa_power)``` method with your chosen radio parameters in the setup function. Afterwards, Arduino strings and char arrays can be transmitted via the ```transmit(message)``` method, while byte arrays can be transmitted using the ```transmitB(message, msgSize)``` method, or the radio can listen for incoming messages using the ```listen(max_wait_time)``` method, which will listen for a byte array message for up to ```max_wait_time``` milliseconds. To switch the radio between receiving and sending mode, you can use the ```changeFrequency(frequency)``` method. 

## Configuration file parameter list
A complete table of all the configurable parameters of the OLB is given below. Do note that the default time unit of the system is milliseconds, and hence all time values in other time units are scaled. 
| Parameter | Default value | Description |
| --------- | ------------- | ----------- |
| ```minimal_transmission_period``` | ```5``` | Minimum amount of time in minutes between each data transmission to base station |
| ```LoRa_freq_receive``` | 863 | Frequency in MHz of the reception channel of the drifter. Permitted frequencies subject to local jurisdiction |
| ```LoRa_bw``` | 125 | Bandwidth to be used by the LoRa module n kHz. [Permitted values determined by RadioLib](https://jgromes.github.io/RadioLib/class_s_x126x.html#a2f60df59c80241d98ce078c0417a7f08) | 
| ```LoRa_sf``` | 8 | Spreading factor of the LoRa module. [Permitted values determined by RadioLib](https://jgromes.github.io/RadioLib/class_s_x126x.html#ae5993359ace652fbdc862eb23fdd263d) |
| ```LoRa_cr``` | 6 | Coding rate of the LoRa module. [Permitted values determined by RadioLib](https://jgromes.github.io/RadioLib/class_s_x126x.html#a6f04a13e604d8bd8127ddf910e406ab5) |
| ```LoRa_power``` | 15 | Power use by the LoRa module in dBm. [Permitted values determine by RadioLib](https://jgromes.github.io/RadioLib/class_s_x126x.html#a3836e4459046aef28fb6337fb58be3a9) |
| ```packet_count_send_treshold``` | 4 | Minimum amount of measurements in memory before the OLB can begin looking for a base station |
| ```transmission_grace_period``` | 5 | Cutoff time in seconds before the base station listen time runs out |
| ```max_radio_fix_look_time```   | 90 | Time in seconds the buoy will listen for a nearby base station before resuming other operations |
| ```max_radio_wait_time```       | 40 | Time in seconds the buoy will wait for positive confirmation from a nearby base station before abort transmission attempt |
| ```max_message_length```        | 64 | The maximum possible size in bytes a message that it sent between basestation and buoy can have |
| ```beacon_ping_period```        | 2  | The period in between beacon position transmissions that the buoy sends out |
| ```LoRa_freq_beacon```          | 868 | The frequency, in MHz, of the beacon messages sent by the buoy |
| ```readings_per_measurement```   | 15  | The number of sensor readings that are taken during each measurement cycle |
| ```max_number_of_measurements``` | 40  | The maximum number of measurements the OLB will store in memory at once |
| ```max_GPS_read_time```          | 3   | The maximum amount of time, in minutes, that the OLB will try to get a GPS fix before aborting a measurement cycle |
| ```max_sensor_read_time```       | 40  | The maximum amount of time, in seconds, that the OLB will try to read an onboard sensor during a measurement cycle |
| ```outlier_discard_tolerance```  | 2   | The amount of standard deviations a sensor reading has to exceed from the mean during a measurement cycle, for that reading to be discard if sigma filtering is performed |
| ```GPS_baud``` | 9600 | Baud rate for serial communication with the GPS module |
| ```max_number_of_thermometres``` | 1 | The largest amount of daisy-chained thermometres to a drifter in the deployment |
| ```minimal_measurement_period``` | 10 | The smallest permissible period between each measurement cycle in seconds |
| ```base_measurement_period```    | 300 | The baseline period between each measurement cycle in seconds |
| ```maximal_measurement_period``` | 30 | The greatest permissible period between measurement cycles in minutes |
| ```scale_factor```               | 100000 | The OLB stores most measurement variables as integers, and the scale factor is the number we scale up a float before casting it to an int. Think of it as the desired amount of significant integers you wish to preserve |
| ```thermometre_pause_between_readings``` | 300 | Time, in ms, between each reading of the thermistors during a measurement cycle | 
| ```remove_outliers```            | true | If true, then the buoy will perform a sigma filter on the recorded readings before storing the mean value as a new measurement |
| ```debug_serial```               | true | Disable to turn off logging to serial |
| ```enable_GPS```| true | Disable to deactivate the GPS. Useful only for debugging | 
| ```enable_watchdog``` | true | If true, then a watchdog will ensure the buoy will reset if a software lock is to occur |
| ```debug_SD``` | false | Print debug info to SD card |
| ```serial_baud``` | 115200 | Baud rate of the OLB communication over Serial port |
| ```enable_motion_detection``` | false | Enable or disable adaptive mesurement period based on measured buoy movement |
| ```transmitDeploymentMessage``` | false | Enable to force the buoy to get a positive in range confirmation before proceeding to the loop part of the program |
| ```debug_LED_enabled```         | false | Turn on flashing of the onboard LED based on where the OLB is in the program cycle |
| ```sleep_GPS```     | true | Turn on to activate power management of the GPS. Recommended to leave on |
| ```perform_handshake``` | true | If disabled, then the OLB will no longer wait for confirmation from a BST before sending its data. |
| ```enable_baseStation_parameter_updates``` | false | Allow the base station to send updates to configuration parameters during deployment |
| ```enable_recovery_beacon``` | true | If enabled, the buoy will regularly send its position on a designated channel for ease of recovery |
| ```log_every_reading```          | true | Flag checking if every sensor reading should be logged to file, or if only the filtered mean value should be stored at the onboard SD card |
| ```resync_RTC_using_GPS```       | true | Flag checking if the buoy should use the GPS satelite time when possible to update its local timestamp |
| ```motion_treshold``` | 0.5 | Speed treshold in m/s before the adaptive measurement period kicks in, if enabled |
| ```target_reading_distance``` |  30 | Desired spatial distance, in meters, between each measurement if the buoy has adaptive measurement period enabled |
| ```watchdog_wait_time``` | 32 | Time, in seconds, the watchdog can wait before it has to be petted. |
| ```sleep_time```   | 9 | Time, in seconds, the buoy will enter into low power mode before waking up to check if it is time for a new cycle and pet the watchdog |
