#include <LiquidCrystal_PCF8574.h>  // 添加應用函式庫LiquidCrystal_PCF8574.h
LiquidCrystal_PCF8574 lcd(0x27);  // 設定液晶顯示模組的 i2c位址，一般情況就是0x27或0x3F其中一個
void setup()
{
  lcd.begin(16, 2);                 // 初始化LCD，寬度16字，高度2行
  lcd.setBacklight(255);         // 設定背景亮度為255
  lcd.clear();                           // 清除顯示器上面的文字
  lcd.setCursor(0, 0);              // 設定文字要顯示的位置在左上角，第幾個字及第幾行(字,行)。
  lcd.print("** first line.");      // 顯示文字(最多16字)
  lcd.setCursor(0, 1);              // 設定文字要顯示的位置在第二行最左邊
  lcd.print("** second line");  // 顯示文字(最多16字)
}
 
void loop()
{
}
