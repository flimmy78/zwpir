# ZWave Motion Sensor

	this project contains zwave pir sensor code.
	stm8 is the co processor program
	zwave is the zwave program.

# Author:   
	au/dlaudience01@gmail.com  


# Use:  

# Developing: 
	- STM8L101  
		1. 检测按键状态,触发ZWave芯片,退网和入网  ok
		2. 检测pir的电平状态,根据需求触发人感的消息   ok
		3. Timer(Count)的设计, 维持定时上报及需求逻辑控制   ok
	- ZM5202  
		TODO..  

# TODO:
	- STM8L101
		1. 当检测到有人的时候,再检测到有人的时候关闭中断
		2. AWU 睡眠时间 优化到 1或者更长,最大103s.  
		3. 当不需要检测时候可以关闭pir 的电源控制角  

# Version:  
	V1.0.0_zwpir_beta0:
		- Functions:  
		- Changes:  
		- Problem:  
		- Todo:  

