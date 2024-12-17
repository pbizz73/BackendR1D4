String value = "101";
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(value);
  if(Serial.available()>0)
  value = Serial.readString();
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:


}
