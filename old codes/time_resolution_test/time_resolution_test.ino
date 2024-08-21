uint32_t systick, t0, t1, t = 0;
uint32_t a, b, c, d, dt = 0;
uint32_t at, bt, ct = 0;
int led = 5;
int in = 8;
float D;
float tt;
float LS = 3.335640; // ns/m
float LSD = 299792458; // m/s
float LSS = 0.299792458; // m/ns
unsigned long i, j;

float my;  // ns   (1/時脈)

void setup() {

  my = F_CPU_ACTUAL;
  my = 1000000000 / my;

  Serial.begin(9600);
  //delay(5000);
  t0 = micros();
  pinMode(led, OUTPUT);
  pinMode(in, INPUT);
}
void loop() {

  // digitalWriteFast test (c=5時脈)
  digitalWriteFast(led, HIGH);
  a = ARM_DWT_CYCCNT;
  while (i == j) {
    if (digitalReadFast(in) == HIGH) {
      b = ARM_DWT_CYCCNT;
      digitalWriteFast(led, LOW);
      c = b - a;
      c = c - 27; //手動扣掉Read的時間
      tt = c * my;
      D = tt / 1000000000 * LSD * 100; //所花時間 轉成秒 *光速距離(m/s) 轉成公分

      Serial.print(c);   //b-a

      Serial.print("\t");
      Serial.print("tt = ");
      Serial.print(tt, 6);
      Serial.print("ns");

      Serial.print("\t");
      Serial.print("light Distance = ");
      Serial.print(D, 6);
      Serial.print("cm");
      Serial.print("\n");
      delay(1000);
      j++;
      break;
    }
  }
  i++;
  /*
    a = ARM_DWT_CYCCNT;
    //digitalWriteFast(led, HIGH);  // digitalWriteFast test (c=4時脈)
    //digitalReadFast(in);  // digitalReadFast test (c=13時脈)
    b = ARM_DWT_CYCCNT; //差3..?
    d = ARM_DWT_CYCCNT;
    LS = LS * 2;
    at = ARM_DWT_CYCCNT;
    bt = ARM_DWT_CYCCNT;
    ct = ARM_DWT_CYCCNT;
    dt = ARM_DWT_CYCCNT;
    digitalWrite(led, LOW);
    c = b - a;
    Serial.print(c);
    Serial.print("\n");
    Serial.print(a);
    Serial.print("\t");
    Serial.print(b - a);
    Serial.print("\t");
    Serial.print(d - b);
    Serial.print("\t");
    Serial.print(at - d);
    Serial.print("\t");
    Serial.print(bt - at);
    Serial.print("\t");
    Serial.print(ct - bt);
    Serial.print("\t");
    Serial.print(dt - ct);
    Serial.print("\n");
    delay(100);*/







  /*
    t = micros();   // 確認是否能抓取最大值(2^32-1)  difficult
    a = ARM_DWT_CYCCNT;
    Serial.print(a);
    Serial.print("\t");
    Serial.print(d);
    Serial.print("\t");
    Serial.print(c);
    if (a > d) {
    c = a - d;
    d = a;
    Serial.print("\t");
    Serial.print(d);
    digitalWrite(led, HIGH);
    dt = 0;
    }
    if (dt > 100000) {
    digitalWrite(led, LOW);
    }
    dt++;
    Serial.println("");*/





  /*
    t = micros();
    a = ARM_DWT_CYCCNT;
    if (t % 1000000 == 0) {  //每一秒讀一次 以此推論時脈頻率
      c = a - b;
      d = c / 1000000;
      Serial.print(" ARM_DWT_CYCCNT = ");
      Serial.print(a);
      Serial.print("\t");
      Serial.print(" t = ");
      Serial.print(t);
      Serial.print("\t");
      Serial.print(" c = ");
      Serial.print(c);
      Serial.print("\t");
      Serial.print(d);
      Serial.print("MHz");
      Serial.print("\n");
      b = a;
    }*/







  /*unsigned int t1, t0 = 0;

    void setup() {
    Serial.begin(1000000);
    pinMode(13, OUTPUT);
    delay(3000);
    }
    void loop() {
    digitalWriteFast(13, HIGH);
    //t0 = SysTick->VAL;
    Serial.print(t0);    Serial.print("\t");
    Serial.print(t1);    Serial.print("\t");
    Serial.print(SYST_CSR);    Serial.print("\t"); //3 or 65539
    Serial.print(SYST_CSR_COUNTFLAG);    Serial.print("\t");
    Serial.print(SYST_CSR_CLKSOURCE);    Serial.print("\t");
    Serial.print(SYST_CSR_TICKINT);    Serial.print("\t");
    Serial.print(SYST_CSR_ENABLE);    Serial.print("\t");
    Serial.print(SYST_RVR);    Serial.print("\t");
    Serial.print(SYST_CVR);    Serial.print("\t"); //micros? 0~99
    Serial.print(SYST_CALIB);    Serial.print("\t");
    Serial.println("");
    digitalWriteFast(13, LOW);
    delayMicroseconds(t1);
    t0++;
    if (t0 % 50 == 0) {
      t1++;
    }
    if (t1 > 1024) {
      t1 = 0;
    }
    }*/






}
