Regenbox
========

Main repo for regenbox project.

firmware/regenbox/regenbox.ino:
  Upload the sketch to arduino board connected to regenbox circuit using Arduino IDE (http://arduino.cc).
  Use Serial monitor or any other serial terminal to send command and retrieve datas.
  Minicom usage : minicom -D /dev/ttyUSB0 -b 9600 -C test.log
  With minicom, you can directly save the measurements in a file using the -C option.
  To leave minicom press CTRL + A and x

