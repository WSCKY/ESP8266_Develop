HOWTO:
just run ./gen_misc.sh, and follow the tips and steps.


Compile Options:
(1) COMPILE
    Possible value: xcc
    Default value:
    If not set, use gcc by default.

(2) BOOT
    Possible value: none/old/new
      none: no need boot
      old: use boot_v1.1
      new: use boot_v1.2
    Default value: new

(3) APP
    Possible value: 0/1/2
      0: original mode, generate eagle.app.v6.flash.bin and eagle.app.v6.irom0text.bin
      1: generate user1
      2: generate user2
    Default value: 0

(3) SPI_SPEED
    Possible value: 20/26.7/40/80
    Default value: 80

(4) SPI_MODE
    Possible value: QIO/QOUT/DIO/DOUT
    Default value: DIO

(4) SPI_SIZE_MAP
    Possible value: 0/2/3/4/5/6
    Default value: 6

For example:
    make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=DIO SPI_SIZE_MAP=6
