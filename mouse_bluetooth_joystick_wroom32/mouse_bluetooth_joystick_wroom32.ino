// Mouse Bluetooth con ESP32-WROOM-32 (DevKit V1, 30 pines) + joystick
// Mover el joystick: mueve el cursor (mientras más lo empujas, más rápido)
// 1 pulsación corta del joystick: clic izquierdo
// 2 pulsaciones rápidas del joystick: doble clic izquierdo
// Clic derecho: NO existe por ahora, no se usa en este código
//
// Librerías (Arduino IDE > Herramientas > Gestionar librerías,
// instalar en este orden):
//   1. NimBLE-Arduino, de h2zero, versión 2.3.8 o mayor
//   2. HijelHID_BLEMouse, de hijelhub
// NO uses la librería clásica BleMouse de T-vK: no compila con el
// core ESP32 3.3.12, igual que pasaba con BleKeyboard.
// Placa: DOIT ESP32 DEVKIT V1  o  ESP32 Dev Module
//
// Conexiones (joystick a 3.3V, NO a 5V, el ADC es de 3.3V):
//   Joystick VCC  -> 3V3
//   Joystick GND  -> GND
//   Joystick VRx  -> D32  (GPIO 32, ADC1)
//   Joystick VRy  -> D33  (GPIO 33, ADC1)
//   Joystick SW   -> D4   (GPIO 4, a GND al pulsar, pull-up interno)
// Si el cursor se mueve al revés, intercambia el signo de ese eje
// en calcularVelocidad() o en la llamada a mouse.move().

#include <HijelHID_BLEMouse.h>

HijelBLEMouse mouse("ESP32-WROOM-Mouse", "Crucito", 100);

const uint8_t PIN_VRX = 33;  // eje X: izquierda / derecha
const uint8_t PIN_VRY = 32;  // eje Y: arriba / abajo
const uint8_t PIN_SW  = 4;   // pulsar joystick = clic izquierdo

// Centro del joystick ~2048 (ADC 0-4095). Zona muerta para que
// el cursor no se mueva solo cuando sueltas el joystick.
const int CENTRO = 2048;
const int ZONA_MUERTA = 250;   // 1798 a 2298 = quieto
const int VELOCIDAD_MIN = 1;   // píxeles por reporte, cerca del centro
const int VELOCIDAD_MAX = 12;  // píxeles por reporte, al tope

// Doble clic: 2 pulsaciones en menos de 350 ms = doble clic.
// Si pasa ese tiempo con una sola pulsación, es un clic simple.
const unsigned long DOBLE_CLIC_MS = 350;
unsigned long primerClicTiempo = 0;
bool clicPendiente = false;
bool swAntes = false;
unsigned long ultimoCambioSW = 0;

int calcularVelocidad(int valor) {
  int distancia = valor - CENTRO;
  if (abs(distancia) <= ZONA_MUERTA) return 0;
  int magnitud = map(abs(distancia), ZONA_MUERTA, CENTRO,
                     VELOCIDAD_MIN, VELOCIDAD_MAX);
  magnitud = constrain(magnitud, VELOCIDAD_MIN, VELOCIDAD_MAX);
  return (distancia > 0) ? magnitud : -magnitud;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SW, INPUT_PULLUP);
  analogReadResolution(12); // 0-4095
  mouse.begin();
  Serial.println("Mouse BLE listo. Busca 'ESP32-WROOM-Mouse' en Bluetooth.");
}

void loop() {
  if (!mouse.isPaired()) {
    delay(200);
    return;
  }

  int x = analogRead(PIN_VRX);
  int y = analogRead(PIN_VRY);
  bool sw = (digitalRead(PIN_SW) == LOW); // LOW = pulsado
  unsigned long ahora = millis();

  // --- Mover el cursor ---
  // X: derecha = positivo, izquierda = negativo
  // Y: en pantalla abajo = positivo, arriba = negativo.
  // Al empujar el joystick arriba, VRy baja, por eso se invierte.
  int dx = calcularVelocidad(x);
  int dy = -calcularVelocidad(y);
  if (dx != 0 || dy != 0) {
    mouse.move(dx, dy);
  }

  // --- Clic / doble clic izquierdo, solo al presionar ---
  // Antirrebote: ignorar cambios del botón menores a 50 ms.
  if (sw != swAntes && ahora - ultimoCambioSW >= 50) {
    ultimoCambioSW = ahora;
    if (sw) { // acaba de pulsar
      if (clicPendiente && ahora - primerClicTiempo <= DOBLE_CLIC_MS) {
        mouse.doubleClick(MouseButton::Left);
        clicPendiente = false;
      } else {
        primerClicTiempo = ahora;
        clicPendiente = true;
      }
    }
    swAntes = sw;
  }

  // Si pasó el tiempo de doble clic y solo hubo una pulsación,
  // entonces fue un clic simple.
  if (clicPendiente && ahora - primerClicTiempo > DOBLE_CLIC_MS) {
    mouse.click(MouseButton::Left);
    clicPendiente = false;
  }

  delay(15);
}
