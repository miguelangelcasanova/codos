# 💪CODOS💪

## Un sistema de bajo coste basado en ESP8266/ESP32 para la detección del CO<sub>2</sub> y otras variables ambientales para monitorizar la calidad del aire en el aula (o en otros lugares de trabajo)

*(Este documento está en constante redacción)*

*"Algunos científicos comentan que mejorar la ventilación y la calidad del aire es un método que las escuelas pueden usar para reducir el riesgo de transmisión del coronavirus.
Sin embargo, en una encuesta entre distritos escolares grandes del Norte de Texas, The Dallas Morning News encontró que las escuelas están lejos de alcanzar los parámetros de calidad del aire propuestos en junio por expertos en construcción.  
Investigadores de la Universidad de Harvard recomendaron instalar filtros de aire de alta graduación, limpiadores de aire portátiles y fuentes de luz ultravioleta dentro de los conductos de aire para eliminar al virus.
Al revisar el nivel de dióxido de carbono en las aulas se puede comprobar si está entrando suficiente aire fresco..."*

Fuente: https://noticiasenlafrontera.net/escuelas-no-siguen-recomendaciones-de-calidad-del-aire-parar-reducir-exposicion-a-covid-19/

Otro artículo con información al respecto, (gracias Lina por el enlace):

https://www.caloryfrio.com/construccion-sostenible/ventilacion-y-calidad-aire-interior/colegios-coronavirus-calidad-aire-interior-ventilacion-adecuada-covid-19.html

Existen además evidencias de que los altos niveles de CO<sub>2</sub> influyen sobre el rendimiento de los alumnos en el aula.
https://pubmed.ncbi.nlm.nih.gov/25117890/

Artículos cómo estos y otros me han llevado a elaborar un pequeño dispositivo de bajo coste que permita monitorizar los niveles de CO<sub>2</sub> en las aulas con el objeto de poder medir la concentración de dicho gas y de esta forma saber cuándo tenemos que renovar el aire de un aula para poder seguir de la mejor forma posible las propias indicaciones al respecto de las administraciones públicas españolas:

https://www.miteco.gob.es/es/ministerio/medidas-covid19/sistemas-climatizacion-ventilacion/default.aspx

Si quieres saber más no dejes de leer este interesantísimo hilo en twitter: https://twitter.com/PabloFuente/status/1297457593368088576

Utilizando una hoja de cálculo podemos calcular la cantidad de CO<sub>2</sub>  en función de diversas variables del aula. En el siguiente artículo tenemos una calculadora que permite hacer dicho cálculo, (gracias Mercedes por el enlace):

https://medium.com/@jjose_19945/how-to-quantify-the-ventilation-rate-of-an-indoor-space-using-a-cheap-co2-monitor-4d8b6d4dab44

Este enlace https://schools.forhealth.org/ventilation-guide/ nos dice también cómo y cuánto debemos ventilar...


## ¿Qué es 💪CODOS💪?

💪CODOS💪 es un pequeño circuito electrónico construido sobre un microcontrolador ESP32, un microcontrolador similar a un Arduino pero que ofrece conectividad WiFi y Bluetooth. Esto lo convierte en un dispositivo de Internet de las Cosas, (IoT) lo que nos permite monitorizar los datos de los sensores conectados al mismo a través de Internet. El dispositivo está pensado para medir la cantidad de CO<sub>2</sub> y otros parámetros ambientales para recomendarnos cuando deberíamos renovar el aire de un aula cuando no se disponga de un sistema de ventilación forzada, o bien no sea posible mantener las ventanas abiertas todo el tiempo.

![CODOS es un guiño a hincar los codos en el aula...](img/school_1810350a1-1.jpg)
CODOS es un guiño a hincar los "CO2" en el aula... ;)

* Con un simple Arduino, un sensor de CO<sub>2</sub> y unos led podemos construir un sistema simplificado que permita indicar cuando los niveles de CO<sub>2</sub> están dentro de unos determinado umbrales, esa fue mi primera idea y publicaré también esta versión; pero cambiando el Arduino por un ESP8266 o un ESP32 podemos además enviar los datos a un servidor y monitorizar por ejemplo los datos de distintas aulas de forma centralizada, almacenar datos estadísticos en una base de datos o realizar otras muchas tareas que podrían sernos útiles sin incrementar prácticamente el coste del dispositivo.

### BOM (Bill of materials) / Lista de materiales
En su versión IoT, para construir CODOS se necesitan los siguientes elementos:
- Un ESP32 por ejemplo el ESP32-DOIT-DEVKIT
![ESP32-DEVKITC](img/esp32-devkitc.jpg) 
- Un sensor de CO<sub>2</sub> CC811 (he probado también con otros sensores como el Sensirion SDC30 pero su coste es mucho más elevado)
![Sensor CO2 CC811](img/CCS811.jpg) ![Sensor CO2 CC811](img/CCS811.png)

- Opcionalmente un sensor de humedad, presión y temperatura BME280
![Sensor BME280](img/bme280.jpg)

- Opcionalmente leds de varios colores por ejemplo rojo, naranja y verde para construir un "semáforo" que indique los niveles de CO<sub>2</sub>
![Diodos led](img/leds.jpg)

- Opcionalmente una pantalla OLED SSD1306 u otra (o un ESP32 que la incluya)
![OLED SSD1306](img/OLED-SSD1306.jpg) 

- Necesitarás además cables dupont para conectar entre sí los distintos elementos.
![Cables Dupont](img/cables-dupont.jpg)

- Para alimentar el dispositivo podrás utilizar el puerto USB de un ordenador o mejor un cargador de móvil con conexión microUSB para los ESP o el que corresponga para el Arduino
![Cargador de móvil](img/cargador.jpg)

## Cómo se monta

### Versión Arduino

Vamos a exponer primero de forma sencilla cómo se conecta el sensor de CO<sub>2</sub> CSS811 a un Arduino Nano o UNO, esta versión es la más económica y sencilla del dispositivo. Simplemente hemos de utilizar 5 cables Dupont hembra-hembra o macho-hembra respectivamente y unir los siguientes pines del sensor a otros tantos pines del Arduino:

- Vcc con un cable rojo lo uniremos al pin de 3.3V del Arduino
- GND con un cable negro lo uniremos a uno de los pines GND del Arduino
- SDA se conecta al pin A4 del Arduino
- SCL se conecta al pin A5 del Arduino
- AWake se conecta al otro pin GND del Arduino.

![Conexión del sensor CSS811 al Arduino](img/arduino-css811-conexiones.jpg)

Luego simplemente hemos de conectar un cable USB y podremos programar el Arduino con el código necesario para poder leer los datos del sensor. 

En esta versión del dispositivo los datos sólo pueden monitorizarse a través de un ordenador conectado mediante dicho cable USB, por eso en la versión definitiva utilizaremos un ESP8266 o un ESP32 que funcionan de forma similar pero permiten además enviar los datos vía WiFi y en el caso del ESP32 también vía Bluetooth.

### Version ESP32

La conexión de los sensores es muy sencilla, tanto el sensor de CO<sub>2</sub> como el sensor ambiental utilizados utilizan conexiones i2c, es decir basta con alimentarlos a 3.3V y masa y conectar a los GPIO22 y GPIO21 que en el ESP32 corresponden a las conexiones SCL y SDA del mencionado protocolo respectivamente. Si deseas conectar la pantalla OLED se conecta también en estos mismos pines.
Dado que podemos utilizar dos pines para conectar varios sensores o la pantalla necesitaremos utilizar una placa de prototipos o diseñar una placa de circuito impreso para conectarlos todos en el mismo punto.

Para la conexión de los diodos led al tratarse de salidas de 3.3V deberíamos utilizar resistencias limitadoras de corriente y conectarlos a través de estas a cualquiera de los GPIO, yo he escogido los GPIO9, 10 y 11. Al conectar los diodos led hemos de tener en cuenta su polaridad.

### El programa
También he diseñado varias versiones del programa según la plataforma utilizada. 
El programa debe cargarse desde el entorno IDE de Arduino o desde VS Studio Code en la placa correspondiente.

### El dispositivo
He diseñado una caja imprimible en 3D para poder albergar el dispositivo aunque este puede montarse directamente sobre una placa de prototipos si no se tiene la habilidad para soldar unos cuantos componentes aunque su montaje debería resultar especialmente sencillo.

### Usando el dispositivo
El dispositivo se conecta automáticamente a la red del aula para permitir que los datos de los sensores pueden visualizarse en una página web que genera el dispositivo desde cualquier otro dispositivo conectado a la misma red. Para ello debes averiguar la dirección IP del dispositivo y abrir en tu navegador una URL del tipo siguiente: http://192.168.1.105 dónde los números indican la dirección IP local del dispositivo en la red local. 

![CODOS](img/Codos.png)

### Preguntas frecuentes

#### ¿Cuál es el objetivo del proyecto?
Dotar a las aulas y otros espacios de trabajo de una forma sencilla y económica 
de medir la calidad del aire, en concreto de la concentración de CO<sub>2</sub> 

#### ¿Dónde comprar los componentes?

El ESP32 y los leds se pueden comprar en muchas tiendas físicas de electrónica en España o a través de Internet. En China por supuesto resultan mucho más económico; pero tardarás en tenerlo varias semanas en tener los componentes en tus manos.
Los sensores son un poco más difíciles de localizar en tiendas físicas pero puedes adquirirlos igualmente en China o un poco más caros encontrarlos a través de ebay o Amazon.

El ESP32 lo puedes comprar en España por unos 10€ por ejemplo en: 

https://www.ebay.es/itm/EL0116-ESP-WROOM-32-ESPRESSIF-Placa-Desarrollo-Arduino-WiFi-Bluetooth-Dual-Core/233565682462

En la misma tienda puedes comprar los LEDs y unos cables Dupont hembra-hembra.

En ebay y en Amazon hay muchas tiendas que te ofrecen el sensor de CO2 o el de humedad y temperatura pero su coste es mucho más elevado que pidiéndolo a China:

https://www.ebay.es/itm/CCS811-Carbon-Monoxide-CO-VOCs-Air-Quality-Numerical-Gas-Sensors-CJMCU-811/323688562130

https://www.amazon.es/TECNOIOT-Monoxide-Quality-Numerical-CJMCU-811/dp/B07RGLMS1J

Este es otro modelo que resulta también muy económico:

https://www.amazon.es/KEYESTUDIO-Quality-Arduino-Monoxide-Numeric/dp/B086HCSM6N/ref=sr_1_1?__mk_es_ES=%C3%85M%C3%85%C5%BD%C3%95%C3%91&dchild=1&keywords=ccs811&qid=1598700075&refinements=p_85%3A831314031&rnid=831276031&rps=1&sr=8-1

Comprando 5 unidades del ESP32 te salen a 6€ en el siguiente enlace:

https://www.amazon.es/gp/product/B074RG86SR

En Aliexpress últimamente están entregando en 10 días (Hoy es 29/08/2020)

https://es.aliexpress.com/item/32903358923.html?spm=a2g0o.productlist.0.0.26bc4071sE7mf2&algo_pvid=159e700e-7ec4-41f6-a8b4-ef1eb37b29d2&algo_expid=159e700e-7ec4-41f6-a8b4-ef1eb37b29d2-0&btsid=0b0a0ad815986989110232476e8172&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_

*(Este documento está en constante redacción)*


