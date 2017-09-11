
Install docker using official how-to: https://docs.docker.com/engine/installation/


Build ssn_bsp image using make setup

Build reference  Arduino application for your board:

    make build BOARD=[mzero, mzeropro, due, dueusb]

Flash reference Arduino application on your board:

    make flash BOARD=[mzero, mzeropro, due, dueusb]
