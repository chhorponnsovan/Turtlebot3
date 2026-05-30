# **Encoder/Wheel Info and Calibration**













## **Wheels and Encoder specification**


Wheel Diameter : 0.069 m


Ticks per revolution : $$(410 \times 2)$$ tick/rev


Wheel cirfumference : $$\pi \times 0.069m$$ 


Meters Per tick = $$0.069/(410 \times 2)$$


Right wheel correction PWM = $$1.0/1.015$$ (not sure)


Left motor :
 - Enable Pins     : 13 
 - H bridge IN1    : 12
 - H bridge In2    : 14


Right motor:
 - Enable Pins     : 21 
 - H bridge IN1    : 18
 - H bridge In2    : 19 


PWM Config 
- PWM frequency    : 20 000 Hz
- PWM Resolution   : 8 (255 bits)
- PWM Channel Left : 0
- PWM Channel Right: 0

For more info about the schematic check (Esp32 30 pin diagram.pdf)






