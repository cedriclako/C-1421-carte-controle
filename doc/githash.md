# Git Hash

On startup, the STM32 will print the git commid ID and branch.
It allows to know what program is running and what code was used for the binary file.

Using the git describe command in the source code folder:

![](./assets/githash-cmd-example.png)

In debug serial on the STM32 :

![](./assets/startup-githash-stm32.png)

On the webpage on the ESP32 :

![](./assets/startup-githash-esp32.png)

