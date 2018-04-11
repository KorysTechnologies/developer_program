FROM python:2.7

RUN useradd -ms /bin/bash ssn
RUN adduser ssn dialout
RUN adduser ssn plugdev
ADD https://raw.githubusercontent.com/platformio/platformio-core/develop/scripts/99-platformio-udev.rules /etc/udev/rules.d/99-platformio-udev.rules
RUN chmod 644 /etc/udev/rules.d/99-platformio-udev.rules

RUN pip install platformio==3.4.1
USER ssn:root

ENV CI=true

CMD ["/bin/bash"]
WORKDIR /home/ssn/
