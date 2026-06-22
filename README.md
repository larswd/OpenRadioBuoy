# The Open Radio Buoy (ORB)
This repository contains the firmware, list of hardware and PCB schematics that collectively make up the Open Radio Buoy (ORB), as described in our paper which you can read [here](https://arxiv.org/abs/2601.05615). The buoy is designed to be a low-cost, open-source drifter for coastal water measurements. The archive is currently incomplete, but more updates will come. 



## List of hardware components

### Drifter

A single ORB drifter's electronics is made up of the following components, with links to manufacturer website. 


| Quantity | Component | Note |
| -------- | --------- | ---- |
| 1 | [Seeed Studio Wio-E5 mini](https://wiki.seeedstudio.com/LoRa_E5_mini/) | Comes with actory protection which needs to be disabled manually |
| 1 | [Adafruit Ultimate GPS](https://www.adafruit.com/product/746)          | Comes with a holder for a CR1212 clock battery to remember last fix.| 
| 1 | [Adafruit SD Breakout](https://www.adafruit.com/product/254)           | | 
| 1 | [DS18B20 temperature sensor](https://www.sparkfun.com/temperature-sensor-waterproof-ds18b20.html) | |
| 1 | [Pololu 3.3 V Step up/Step down voltage regulator](https://www.pololu.com/product/2122) | |
| 1 | 10 kOhm resistor  |
| 1 | 4.7 kOhm resistor  |
| 2 | 33.0 Ohm resistors |
| 1 | Magnetic switch         | Normally open is *strongly* recommended. Not mandatory. |
| 1 | [CAB-14574 connector](https://no.mouser.com/ProductDetail/474-CAB-14574) | Not mandatory, but makes arming and disarming the drifter easier |
| 1 | Strong magnet | To turn off the buoy from outside the buoy container | Not mandatory |

As for battery power, the buoy works with any kind of power source with a voltage between 2.7 and 11.8 V, as per the Pololu documentation. This can be either a Lithium battery such as [Saft LSH20](https://www.nkon.nl/novat/saft-lsh-20-lithium-battery-3-6v.html), although any user should be aware of the [risks of using Lithium powered batteries](https://www.relionbattery.com/blog/the-truth-about-lithium-batteries-and-safety). Alternatively, several alkaline batteries (as was done by the [SFY](https://github.com/gauteh/sfy).) in series can be utilised without need of modifying the circuit. 


### Base station

A single ORB base station electronics is made up of the following components, with links to manufacturer website. 


| Quantity | Component | Note |
| -------- | --------- | ---- |
| 1 | [Seeed Studio Wio-E5 mini](https://wiki.seeedstudio.com/LoRa_E5_mini/) | Comes with actory protection which needs to be disabled manually |
| 1 | [Adafruit SD Breakout](https://www.adafruit.com/product/254)           | Not mandatory |
| 1 | [Pololu 5 V Step up/Step down voltage regulator](https://www.pololu.com/product/4082) | |
| 1 | [CAB-14574 connector](https://no.mouser.com/ProductDetail/474-CAB-14574) | Not mandatory, but makes arming and disarming the drifter easier |
| 1 | [Blues Notecard Cellular](https://shop.blues.com/collections/notecard/products/notecard-cellular?variant=44541263118577) | Check the coverage diagram for your area before ordering |
| 1 | [Blues Notecarrier A](https://shop.blues.com/products/carr-al) | |
| 1-2 | Capacitor (One [1F](https://no.mouser.com/ProductDetail/Eaton-Electronics/KVR-5R0V105-R?qs=4ASt3YYao0XQhknEZPCgzw%3D%3D) or two [10F](https://no.mouser.com/ProductDetail/Knowles-Illinois-Capacitor/DGH106Q2R7C?qs=vmHwEFxEFR9tIw5tsmf9eg%3D%3D)) | Capacitor strength dependent on power source |

If you choose to power the base station with Lithium batteries like the aforementioned LSH20, then a single 1 F capacitor is sufficient to keep the base station operational. If, however, you use alkaline batteries as your power source then 10 F capacitors are required to ensure that the high energy spikes drawn by the GSM module can be met. 

### Other components
To code the WiO E5 mini, remember to order at least one [ST link](https://www.adafruit.com/product/2548) and some [Female-Male jumper cables](https://no.rs-online.com/web/p/breadboard-jumper-wires/2048242) in order to be able to upload the software to the buoys and base stations. 

## Assembly instructions

### Electronics assembly - Buoys
Order the [PCBs](https://github.com/larswd/OpenRadioBuoy/tree/main/hardware/buoy_PCB) by uploading the Gerber files folder to your favourite PCB manufacturer, such as [JLCPCB](https://cart.jlcpcb.com/quote?spm=jlcpcb.Public.2006) or [Seeed studio](https://www.seeedstudio.com/fusion_pcb.html). Then order the components listed above according to how many buoys you plan to make. They can be ordered either from the manufacturer, and in many cases from a local electronics dsitributor. 

The assembly can begin once you have all the components at hand. The PCBs offered are currently Through-Hole PCBs, and requires therefore manual soldering. As both sides of the PCB is utilized in somewhat overlapping segments, it is vital that some components are soldered before others. I would recommend soldering in this order: 

1. Wires to the power pins at the bottom of the PCB
2. Pololu
3. Trim the pins on the back of the Pololu, then tape. 
4. SD card writer
5. Trim the pins on the back of the SD card writer, then tape.
6. GPS
7. Trim GPS pins
8. Wio E5 mini
9. MF pin sockets for the programmable pins
10. DS18B20
11. Resistors
12. Magnetic switch
13. Battery connectors
    
### Electronics assembly - Base stations
Order the [PCBs](https://github.com/larswd/OpenRadioBuoy/tree/main/hardware/BST_PCB) by uploading the Gerber files folder to your favourite PCB manufacturer, such as [JLCPCB](https://cart.jlcpcb.com/quote?spm=jlcpcb.Public.2006) or [Seeed studio](https://www.seeedstudio.com/fusion_pcb.html). Then order components according to the amount of base stations you plan on creating. 
The assembly can begin once you have all the components at hand. The PCBs offered are currently Through-Hole PCBs, and requires therefore manual soldering. Unlike the buoy PCBs, there is little overlap between components and they can be soldered on in pretty much any order. Some things to keep in mind

1. The power regulator will somewhat block easy access to the USB port on the LoRa module. Hence, it is strongly recommended to not solder the module directly onto the board. Instead, we recommend soldering on a 1x5 2.54 mm MF pin header, as the power regulator then easily can be removed and reattached if you need to upload software updates. 
2. Super capacitors have a polarity, indicated by the arrows on both the PCB and often on the capacitors themselves. Please check the data sheet of your capacitor before soldering. 
3. The Notecard might need to be initialized before use. A handy guide can be found [here](https://dev.blues.io/quickstart/notecard-quickstart/notecard-and-notecarrier-a/).
4. The notecard sends the data to Blues' Notehub, which is project based. By default, the ORB base station is set to send the data to a notehub project owned by us. We are willing to share any data we receive as well as the url to our dashboard, but if you want to administrate your data yourself, and who gets access to it, then you can create your own project at [https://notehub.io/projects], then you simply exchange the notehub project url at (currently) line 14 in [the GSM module header](https://github.com/larswd/OpenRadioBuoy/tree/main/firmware/common_libraries/gsm/src/gsm.h) with your new project url (which should look something like ```me.proton.larswd:my_cool_project```)


### Installation instructions - Buoy
The installation of the ORB firmware is a two-step process. First, you need to remove the write protection that the WiO-E5-mini comes with out of the box. To do so, you first need a USB C cable and a [ST-Link](https://www.adafruit.com/product/2548). Then, download the [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html#get-software), and install it. Once it is installed, connect the Wio-E5 mini to your laptop via both the USB C cable and with the ST-link. Here, you need to connect the right pins, and we list them as (ST Link pin/Wio pin), 
- (RST/RST),
- (SWCLK/CLK),
- (SWDIO/DIO),
- (GND/GND).
Afterwards, set ```shared``` to ```Enabled``` and ```Reset mode``` to ```Hardware reset```. If you press the circular arrows button, then if everything works you should see under the **OB** header an option called **Read out protection**, set this drop-down menu to AA and press **Apply**. The buoy is now programmable! See a helpful visual guide below: 

![](imgs/CubeProgConfig.png)

The next step is to upload the firmware. If you are happy with the default configuration, then you can jump to the next paragraph. Otherwise, you can configure most important parameters in the [configuration file](firmware/drifter/src/config.h). What each parameter does is described in the [drifter README file](firmware/drifter/README.md).

Once you have a configuration file you are satisifed with, you can upload the firmware to the buoy. To do so, we use [PlatformIO IDE for vscode](https://docs.platformio.org/en/latest/integration/ide/vscode.html), an extension for visual studio code made for programming microcontrollers. Once you have installed platformIO, click **Open project** under the **PIO Home** tab, and navigate to the *firmware/drifter* folder and click **Open drifter**. From here, platformio will open and read the necessary configurations. Once the project has loaded finished loading, which you can see when several new buttons have appeared at the bottom task bar in visual studio code, then you can upload the code. An image of a fully loaded platformio instance is shown below, in particular note the checkmark and arrow key at the very bottom. 
![](imgs/platformio.png)

To upload the code, connect the ORB to your computer both with the ST link (connection instructions are given above) and the USB C cable, then press the upload button (The arrow icon), then wait until the terminal that popped up states something close to 
```
============================================= [SUCCESS] Took 3.12 seconds =============================================
```
### Installation instructions - Base station
The steps for installing the firmware to the base station is more or less the same as the buoy one, with the exception that you instead open the **basestation** project rather than the **drifter** project. Furthermore, you should here remember to go into the ```config.h``` file and set a unique base station ID. See the line where it says:
static constexpr uint8_t base_station_ID            {x};```
Where ```x``` is some number. Set ```x``` to any whole number between 0 and 255 that you have not previously used in your base station fleet, then upload the code to the base station. **REMEMBER**: See bullet point 4 of the assembly instructions for the base station regarding data managegement. 


### Final assembly 

The container components we used are given in the table below 

| Component | Role |
| --------- | ---- |
| [Pipe connector](https://www.biltema.no/bygg/vvs/sanitet-og-vann/avlop/innendors-avlop-gra/innendors-avlop-gra-75-mm/avlopsror-skjotemuffe-75-mm-2000059003) | Main electrical component housing |
| [Lid 75 mm](https://www.biltema.no/bygg/vvs/sanitet-og-vann/avlop/innendors-avlop-gra/innendors-avlop-gra-75-mm/avlopsror-endestopp-75-mm-2000059011) | Top lid |
| [Reduction 75->50mm](https://www.biltema.no/bygg/vvs/sanitet-og-vann/avlop/innendors-avlop-gra/innendors-avlop-gra-75-mm/avlopsror-reduksjon-o-7550-mm-2000065100) | Ballast and battery container |
|[Lid 50 mm](https://www.biltema.no/bygg/vvs/sanitet-og-vann/avlop/innendors-avlop-gra/innendors-avlop-gra-50-mm/avlopsror-endestopp-o-50-mm-2000065102) | Bottom lid |
| [Large cable gland](https://no.rs-online.com/web/p/cable-glands/6694673) | Cable gland for the antenna |
| [Small cable gland](https://no.rs-online.com/web/p/cable-glands/6694660) | Cable gland for DS18B20 |
| [Ballast](https://www.frederiksen-scientific.no/produkt/lodd-med-krok-200-g/191002) | We use 300 grams of tin ballast in each ORB drifter |
| [XPS 300](https://thaugland.no/butikk/byggevarer/isolasjon/mark-grunn-isolasjon/xps/jackon-isolasjon-xps-20mm-300-jackofoam/) | 20 mm isolation foam as a floation device around the top for increased stability. |

 
To assemble, drill two holes in the top lid for the two cable glands. Thereafter, sand the lid to ensure that the top is as uniform as possible. Apply generously with grease on each of the rubber bands on the cable glands and inside the pipes. Insert the fully assembled ORB electronics through the cable glands in the lid. Tighten the antenna gland, but not the thermistor gland, and watch carefully to ensure that the tightening does not loosen the antenna connection to the main circuit board. Insert the bottom lid into the reduction, then place the ballast in the bottom 50 mm diameter pipe, use silicone or other lipophile glue to ensure that it stays put, and add a small cardboard or plastic disc at the top to ensurean even surface. Mark where the magnetic switch is supposed to be placed on the pipe connector with permanent marker, then tape the switch on the other side underneath the marker. Be careful, as improper switch placement means that it is difficult to check if the buoy is turned off while inactive. Cut out a small square or disk of the XPS 300, approximately 15 cm wide at the widest, and cut a 75 mm diameter hole in the middle. Insert the lid into the XPS 300, and then into the pipe connector. Connect the battery to the electronics, and validate that the switch works as intended. Finally, insert the reduction into the connector, and seal the buoy by tightening the thermistor cable gland. 


![](imgs/assembled_ORB.png)
Your ORB drifter is now ready to use. 

### Final assembly - Base station

As the base stations will, most likely, not be floating in water directly, there is significantly more leeway in container design. Pick any water proof container large enough, and drill a single hole large enough for the [cable gland](https://no.rs-online.com/web/p/cable-glands/6694673). In our deployments, we used this [fishing box](https://www.clasohlson.com/no/Vanntett-boks-/p/31-8544) which could fit three D Cell batteries and the base station, although it was a bit too cramped. Hence, we recommend finding a slightly larger container if possible. 

## Images and deployments
Images from the deployments will come soon. 

## A small thanks 
We would also like to thank the authors of the many open source libraries that collectively form the code base of the Open Radio Buoy. In no particular order, we wish to express our gratitude to 
- [Paul Stoffregen and contributors' OneWire](https://github.com/PaulStoffregen/OneWire)
- [Mikal Hart and contributors' TinyGPSplus](https://github.com/mikalhart/TinyGPSPlus)
- [Jan Gromeš and contributors's Radiolib](https://github.com/jgromes/RadioLib)
- [Bill Greiman's SdFat](https://github.com/greiman/SdFat)
- [Jean Rabault's OpenMetBuoy](https://github.com/jerabaul29/OpenMetBuoy-v2021a)
