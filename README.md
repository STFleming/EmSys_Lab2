# EmSys Lab 2: Timers, Interrupts, and FreeRTOS

For this lab we will be using a lightweight measurment library (let-esp32) that I have developed to instrument our TinyPico devices. In your groups you will use the let-esp32 to perform experiments to answer questions about TinyPico, you will also learn about the following:

* Configuring hardware timers
* Interrupts
* FreeRTOS

There are three videos associated with this lab:

* [[Measuring events on the TinyPico with let-esp32](www.youtube.com)] __not yet published__
* [[Configuring Hardware Timers and Interrupts](www.youtube.com)] __not yet published__
* [[FreeRTOS](www.youtube.com)] __not yet published__

# Logbook structure

We will use the same git repositories setup for Lab1. It was a bit of hassle getting these setup for the first lab so we may aswell keep the same repositories now that everyone is setup. 

Please do the following to your current git logbook repository:
* Make a directory called `Lab2/` in the root directory of your repository
* Make a file `Lab2/README.md` this is where you will write about your experiments and results.
* Make a directory `Lab2/src` this is where you will contain the source code for your various experiments, it is up to you to keep this directory organised and use it to support what you write in `Lab2/README.md`


# Setting up your environment for Lab2

You should not need to do anything, simply log into your linux lab-machine. If your environment is configured correctly from the previous lab, it should pull in the latest version of the let-esp32 and install it on your machine.

## Measuring your TinyPico

To perform detailed timing measurments on the TinyPico we will use `LetESP32.h` a header file that will automatically be installed on your lab machine when you log in. This library implements something called a tracebuffer that uses one of the accurate hardware timers on the ESP32 to perform highly accurate measurments. The measurments get collects in a small buffer that periodically get flushed to the webserver where they can be viewed graphically or downloaded textually (comma separated file format `.csv`).

Each group will have a unique page that only their device can send data to using their dotDevice ID. They will be able to configure the tracer to measure different parts of their system and view and download results just for their device.

To use the library include the following at the top of your program

```C
#include "LetESP32.h"
```

We can then instantiate a trace buffer with the following command where we give it: ``ssid``, the ID of the wireless network in the lab; ``password``, the password for the wireless network in the lab; ``ws``, the websocket address and port for the websocket server in the lab; and finaly ``dd_id``, the dotDevice ID for your group. All of the parameters, apart from the ``dd_id`` you can leave the same.

```C
const char* ssid = "NETGEAR35";
const char* password = "magicalfinch482";
const char* ws = "ws://192.168.1.2:1234";
const char* dd_id = "wibble00";

LetESP32 tracer(ssid, password, ws, "wibble00");
```

This creates ``tracer`` our tracebuffer that we will use to measure the specific timings that event happen in our system.

However, we are not _quite_ ready to start using it to measure our system. We need to do one last thing to connect our trace buffer to the network.

```C
void setup() {
   tracer.connect();
}
```

This will connect it to the WiFi network in the lab.


Using the ``tracer`` is pretty straightforward. We simply pass in an event ID, which is a 16 bit ``unsigned int``, and tracer will record the specific time when that event occurred using the built-in hardware timer. 

```C
    tracer.event(42);
```

For instance, the code above will record the accurate time that the event (ID 42) occurred. Once the tracebuffer has recorded 256 events, it then flushes the events to the central webserver where you'll be able to view and download the data.

Putting this all together you get the following:

```C
#include "LetESP32.h"

const char* ssid = "NETGEAR35";
const char* password = "magicalfinch482";
const char* ws = "ws://192.168.1.2:1234";
const char* dd_id = "wibble00";

LetESP32 tracer(ssid, password, ws, "wibble00");

void setup() {
   tracer.connect();
}

void loop() {
   delay(1);
   tracer.event(42);   
}
```

The code above will record an event with id ``42`` every 1ms. 

### Viewing and downloading the tracebuffer data

To view the trace of my device I need to visit a special page for my group.
My dotDevice ID is ``wibble00`` this corresponds to my group ID ``90``. So if I visit the page [[http://ec2-52-15-138-171.us-east-2.compute.amazonaws.com:4000/G90](http://ec2-52-15-138-171.us-east-2.compute.amazonaws.com:4000/G90)] Then I will see the tracebuffer data for my device, Notice the G90 at the end. If my group ID was 34 then it would be `/G34` at the end. **For groups 0-9 use /G00, /G01, /G02. /G03....** 

Notice the server does ocassionally go down, so please check this [[page]()] to get the latest address.

Running the code above, and visiting the appropriate page for our group we get the following:
![](imgs/single_event_trace.gif)

We can pause the trace using the pause/play button, zoom in using the mousewheel, and see the key of events down the bottom. (__There is currently a slight bug with the X-axis at the top when the stream is paused, you should believe the numbers on the dashed marker instead__)

Let's add some more events:
```C
#include "LetESP32.h"

const char* ssid = "NETGEAR35";
const char* password = "magicalfinch482";
const char* ws = "ws://192.168.1.2:1234";
const char* dd_id = "wibble00";

LetESP32 tracer(ssid, password, ws, "wibble00");

void setup() {
   tracer.connect();
}

void loop() {
   delay(1);
   tracer.event(42);   
   delay(1);
   tracer.event(32);   
   delay(1);
   tracer.event(22);   
}
```

Visiting our groups page now we get the following trace:

![](imgs/multi_event_trace.gif)

We can see that the number of event types has increased as we expect. Each event gets added to it's own row, and the key tells us the mapping from colour to ID.

### Accuracy of the event tracer

As mentioned previously, the event tracer uses custom hardware timer peripherals to capture the precise times the event happens. This timer is a 64 bit timer that counts at 40MHz. This means that each event measurement records a 64bit integer count of the number of times the 40MHz input oscillated. This gives the timer an accuracy of ``1/(40*10^6)``, however, there will be overheads in calling ``tracer.event()``.


### Downloading RAW data

The visual representation on the webpage is great for inspecting our signals and debugging, however, it probably is not very accurate if we want to perform detailed measurements. For that reason you can download a comma separated variable (`.csv`) file of the data for processing in the language or spreadsheet software of your choice.

To download the data, just append ``.csv`` to your groupname at the end of the url you visit to view the trace data.

![](imgs/raw_data_download.gif)

In this CSV file each line has two variables, ``event ID``, ``64 bit timer count``. This means that to get the _real_ time you have to multiple the count by ``1/(40*10^6)``.


## Question 1: what are the overhead of calling ``tracer.event()``? 

Design an experiment to measure the overhead of ``tracer.event()`` as accurately as you can. You should take multiple measurements and average them. Commit your experiment code and discuss your experiments and results in `lab2/README.md`.

## Question 2: what are the overhead of ``loop()``?

Design an experiment to measure the overheads of the ``loop()`` call in the Arduino program as accurately as you can. You should take multiple measurements and average them. Commit your experiment code and discuss your experiments and results in `lab2/README.md`.



