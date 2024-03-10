## About the Project

For my ECE 198 project, I designed and implemented a temperature and humidity regulator for patients with chronic obstructive pulmonary disease (COPD) to minimize the occurrence of exasperated, life-threatening symptoms. Once turned on, the system performs the following functions:

* Passively measures the ambient temperature and relative humidity

* Detects parameters that fluctuate significantly from the predefined range (based on multiple sources)

* Wirelessly interfaces with home appliances to adjust either parameter accordingly

### Built With

Software:

* C

Hardware:

* STM32 Nucleo-F401RE

* DHT22 Digital Temperature and Humidity Sensor

* HC06 Bluetooth Module

* Home Appliances with Bluetooth Control (listed below)

## Getting Started

### Requirements

The system requires the following Bluetooth-compatible appliances to function most effectively:

* Heating Unit

* Cooling Unit

* Humidifier

* Dehumidifier

### Installation Procedure

The system can be installed anywhere near a power source. Simply attach the MCU to a power supply and connect all appliances listed above via Bluetooth.

## Creators

Sean Zhang
