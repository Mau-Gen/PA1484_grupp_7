# PA1484 – Software Development: Project Group 7
## Introduction
- Our system is a weather app that can view both the forecast and historical data with the help of a table and graph, it is also able to check for different cities and temperatures.
- Our system solves the problem of not being able to check on the weather for the upcoming days and for how the weather has been the latest months for different cities and weather parameters.
- The system used an ESP32 microcontoller with a touchscreen to be able to view the contents of the system. The weather data was fetched from the SMHI API and the building of the system was through platformIO. 
- The main functionality of the program is to show different cities weather conditions based on what the user chooses to view.

## Getting started

### Prerequisites
To be able to start making a weather app like ours you will need an ESP32 microcontroller with a lilygo amoled display, a computer and some knowledge in programming.

### Installation
You can go to the forked repo to get all the needed base code that will make it possible to run the ESP32 controller together with it's dependencies. You will need an IDE like VS Code or something similiar. You will need to install platformIO and compile the program using a C++ compiler.

## Building and running

- To build your base code you will need to first go to the platformIO interface and press Build All, this will compile the source code to machine code. To upload your build you have to connect the ESP32 to your computer and then but it in upload mode which you do by pressing BOOT and then RST and then letting go of RST and then BOOT. When you have done this you can press Upload All in your platformIO interface.
- You start the ESP32 and your code will then start running which will make you able to view what your code has actually done.
- The system is static and won't accept different arguments or modes.
- To operate our system you can slide with your finger on the touchscreen to get to the different tiles that have different contents. You can press the settings button on the 2nd and 3rd tile to go the the settings tile which will make it so that you can decide what city or parameter you want to view. 

## Features
These are the user stories that we have completed and not completed:
- [x] US 1.1C: As a user, I want to see a starting screen to display the current program version and group number on the first screen.
- [ ] US 1.2C: As a user, I want to see the weather forecast for the next 7 days for the selected city on the second screen in terms of temperature and weather conditions with symbols (e.g., clear sky, rain, snow, thunder) per day at 12:00.
- [x] US 2.1: As a user, I want to be able to navigate between different screens (like forecast
screen) by sliding a finger over the touch screen.
- [x] US 3.1: As a user, I want to have a screen to view historical weather data.
- [ ] US 3.2D: As a user, on the third screen I want to view the latest months (SMHI API
period: latest-months) of historical hourly data for selected weather parameter in the
selected city, using a slider to interact with the historical graph by scrolling where a
depleted slider corresponds to the oldest datapoint and a full slider corresponds to the
latest datapoint
- [x] US 4.1: As a user, on the fourth screen, I want to access a single settings screen to
configure both the city and weather parameter options.
- [x] US 4.2: As a user, I want to select weather parameters (e.g., temperature, humidity,
wind speed), using a dropdown list, to customize the historical graph.
- [x] US 4.3B: As a user, I want to select from five different cities, namely
Karlskrona(65090), Stockholm(97400), Göteborg(72420), Malmö(53300), and
Kiruna(180940), using a dropdown, to view their weather data for the historical data
and starting screen forecast.
- [x] US 4.4: As a user, I want to reset the selected city and weather parameter to default
using a button.
- [x] US 4.5: As a user, I want to set my default city and weather parameter to the current
selection using a button, so they are automatically selected when I start the device.
- [x] US 4.6: As a user, I want the device to store my default city and weather parameter so
that they are retained even after a restart.
- [x] US 4.7: As a user I want to have button on the forecast and historical weather data screens that takes me to settings and then after I've chosen my settings I want to go back to the normal screen.
