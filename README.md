# programrbf

This program its for write a RBF file using GPIO ports of RPI (Raspberry Pi) to Cyclone Altera FPGA.

The big work was made from Victor Trucco, so this program can't was possible without it. You can check source work here:
[STM32 Firmware Gitlab Victor Trucco](https://gitlab.com/victor.trucco/Multicore/-/tree/master/System/STM32)
**Thanks Victor for your great work.**


### **Instruction**

I used a RPI ZERO with Raspbian but its must work in any RPI (just need check on RPI 4).

If you want compile just need install wiringPi:
>sudo apt-get install wiringpi

If you want use it on Cyclone II you need uncomment a line if not left uncomment:

>//if (bitcount<45) continue;
to
>if (bitcount<45) continue;

For compile just need add library at the end:

>g++ programrbf.cpp -lwiringPi

And last for use you need give / pass arguments, first rbf file and after gpio ports TCK TDI TMS and TDO:

>./programrbf_v01 rbf_file TCK TDI TMS TDO

Please take care choosing port and use under your risk.

### **Media contact**

If you want contact us you can found us here:

* [Telegram group](https://t.me/CYC1000)
* [Discord channel](https://discord.gg/YDdmtwh)
