# Smart-farming-system
Smart Farming System A smart farming system uses sensors to monitor soil, temperature, light, and other field conditions. It helps farmers monitor crops efficiently and make better decisions for irrigation and farm management.PROBLEM STATEMENT

Farmers, especially those with remote or large fields, have to frequently visit their farms manually to check soil moisture, crop health, and irrigation conditions. This process is time-consuming, physically demanding, and may lead to delayed irrigation when farmers cannot reach the field on time. In addition, the lack of reliable internet and electricity in remote farming areas makes continuous crop monitoring difficult.

COMPONENTS

Arduino UNO board
Soil moisture sensor
Rain sensor module
16×2 LCD(Liquid crystal display) with 12C module
Temperature sensor
Smoke and gas detection sensor
Ultrasonic sensor
IR sensor(Human detection)
5V Relay module
Bluetooth module(HC-05)
Breadboard
LED(Light emitting diode)
Resistor
Jumper wires
USB cable
Water pump motor
Mobile phone for monitoring



BLOCK DIAGRAM



WORKING FLOW

Sensor monitoring: The soil moisture, temperature/humidity,rain detection,LDR,smoke sensor these sensors continuously collect information from the form.
Data processing: The Arduino UNO receives and processes all sensor readings.
Local display: The collected sensor values are shown on the 16×2 LCD display.
Data transfer: The Arduino sends the sensor information through the Bluetooth module(HC-05).
Ultrasonic sensor: It is used to measure the distance of an object/person.
LDR sensor: During day time when sunlight is sufficient light is present the LED remains OFF.
 During night time if any object/person is detected the LED gets ON and buzzer will intimate it and through bluetooth we can monitor it. 
Mobile monitoring: The farmer receives and monitors the farm data on a mobile phone using a bluetooth app.

Pump control: Based on the required irrigation condition, the control Arduino operates the relay module.
Irrigation: The pump provides water to the crops, helping maintain proper irrigation and avoid unnecessary water usage.

CIRCUIT DIAGRAM


CONCLUSION

         The smart farming system provides an efficient solution for remote farms by continuously monitoring field conditions and sending the information to a mobile phone through bluetooth. Automatic irrigation helps save water, reduce manual work and supports sustainable.   



