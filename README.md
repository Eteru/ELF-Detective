#Installation

1. Install binutils-dev and libiberty-dev.
    * sudo apt-get install binutils-dev
    * sudo apt-get install libiberty-dev
    * also make sure libopcodes is installed on your system

2. Install qt.
    * check https://wiki.qt.io/Install_Qt_5_on_Ubuntu for more details on this.
    * note that this application uses Qt 5.5 and the 5.5 directory is looked for in the Makefile

3. Download and unzip the contents and go to new-version directory.
    * https://github.com/Eteru/ELF-Detective/archive/master.zip

4. Set up path to Qt directory.
    * PATH_TO_QT=/path/to/qt/ && export PATH_TO_QT
    * note that inside the qt folder should be the 5.5 version

5. make

#Usage

./ELFDetective to run the program

* Add an executable file: Click on the first icon or CTRL+E
* Add an object file: Click on the second icon or CTRL+O
* Run the project: Click on the third icon or CTRL+R
* Clear the project: Click on the last icon or CTRL+C
* See instruction hexcodes: Check the hexcodes checkbox

* To inspect any information from the project, click on any symbol / code line from the executable panel.

#Contact

For any issue contact me directly at eteruas@gmail.com
