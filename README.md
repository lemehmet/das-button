# Das Button

A single button with RGB led. When pressed performs an action, checks the result of the action and changes the color accordingly. The code is based on the `simple_http_server` example.

## Prerequisites

1. Install and configure [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)


Troubleshooting:
* `~/.espressif` contains a snapshot of everything, when upgrading (e.g. if python versions do not match anymore, remove that directory before refreshing/reinstalling the sdk)
* On MacOS if the SDK installation fails at cryptographic packages, point the brew openssl:
```bash
 export LDFLAGS="-L/usr/local/opt/openssl@1.1/lib"
 export CPPFLAGS="-I/usr/local/opt/openssl@1.1/include"
 export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"
```
* If the software does not start or goes into a boot loop, check the FreeRTOS unicore setting. The module is `ESP32-SOLO-1` and has a single core available to FreeRTOS.
* Terminate the monitoring session with `Ctrl+]`.

## Using

* Open the project configuration menu (`idf.py menuconfig`) to configure Wi-Fi or Ethernet. See "Establishing Wi-Fi or Ethernet Connection" section in [examples/protocols/README.md](../../README.md) for more details.

* **Important:** The WiFi password is stored in `stdconfig`, make sure it is not shared unintentionally.

* Run the app:
    1. compile and burn the firmware `idf.py -p PORT flash`
    1. run `idf.py -p PORT monitor` and note down the IP assigned to your ESP module. The default port is 80
    1. or combine both steps with `idf.py -p PORT flash monitor`

* Test the endpoints from the `simple_http_server` example:
    1. test the example :
        * run the test script : "python scripts/client.py \<IP\> \<port\> \<MSG\>"
            * the provided test script first does a GET \hello and displays the response
            * the script does a POST to \echo with the user input \<MSG\> and displays the response
        * or use curl (asssuming IP is 192.168.43.130):
            1. "curl 192.168.43.130:80/hello"  - tests the GET "\hello" handler
            1. "curl -X POST --data-binary @anyfile 192.168.43.130:80/echo > tmpfile"
                * "anyfile" is the file being sent as request body and "tmpfile" is where the body of the response is saved
                * since the server echoes back the request body, the two files should be same, as can be confirmed using : "cmp anyfile tmpfile"
            1. "curl -X PUT -d "0" 192.168.43.130:80/ctrl" - disable /hello and /echo handlers
            1. "curl -X PUT -d "1" 192.168.43.130:80/ctrl" -  enable /hello and /echo handlers


* Test the das button functionality:
    * Basic Color  
    Supports only basic color, 8 different colors.  
        `/basic_color`  
        `POST`  
        `Content-Type: application/json`  
        Data: 
        ```json
        {
            "red": [0: False, 1: True],
            "green": [0: False, 1: True],
            "blue": [0: False, 1: True]
        }
        ```
    * 8-Bits Color  
    Sets the 8 bits intensity per color using PWM.  
        `/color`  
        `POST`  
        `Content-Type: application/json`  
        Data: 
        ```json
        {
            "red": [0-255],
            "green": [0-255],
            "blue": [0-255]
        }
        ```
    * Breathe
    Fades in and out using the last set color.
        `/breathe`  
        `POST`  
        `Content-Type: application/json`  
        Data: 
        ```json
        {
            "cycles": [Number of cycles],
            "dur_ms": [Length of each cycle in ms],
        }
        ```
## Details
- The app uses FreeRTOS.
- Button detects rising edges, does minimal debouncing and ignore interrupts during the execution
- The LEDs are connected to PWM and use ESP SDK's LED Controller API. It works but not great. Although not ESP's documentation, this [tutorial](https://www.silabs.com/community/blog.entry.html/2015/07/17/chapter_5_part_4---WneD) explains the concept.
- There is alot of speculation about the ESP module's GPIO silicon bugs, other pins are available but may or may not work.
