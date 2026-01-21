#include <lvgl.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include "NetworkHandler.h"
#include "DoorStatus.h"

/*Set to your screen resolution and rotation*/
#define TFT_HOR_RES   240
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_0
#define STATUS_RESET_TIME 10

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 20 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// code variable 
lv_obj_t *label;
lv_obj_t *img;

//door status
DoorStatus doorStatus;
bool statusChanged = false;
unsigned long lastUpdate = 0;

//scan input
String scannedString = "";
boolean scanComplete = false;

void updateDoorStatusDisplay(lv_obj_t * label, lv_obj_t *img) {
    
    if (statusChanged == false) return; // only run the code when the status changed

    char labelText[64]; // Buffer for dynamic text
	  char img_source[64]; // Buffer for dynamic text

    switch (doorStatus) {
      case DoorStatus::IDLE:
          snprintf(labelText, sizeof(labelText), "Scan QR to open door");
          snprintf(img_source, sizeof(img_source), "S:/idle.jpg");
          break;
      case DoorStatus::LOADING:
          snprintf(labelText, sizeof(labelText), "Loading...");
          snprintf(img_source, sizeof(img_source), "S:/loading.jpg");
          break;
      case DoorStatus::SUCCESS:
          snprintf(labelText, sizeof(labelText), "Door opened ~");
          snprintf(img_source, sizeof(img_source), "S:/success.jpg");
          break;
      case DoorStatus::EQUIP_ERR:
          snprintf(labelText, sizeof(labelText), "Failed: Equipment Error");
          snprintf(img_source, sizeof(img_source), "S:/equip_err.jpg");
          break;
      case DoorStatus::INUSE:
          snprintf(labelText, sizeof(labelText), "Failed: In Use");
      snprintf(img_source, sizeof(img_source), "S:/inuse.jpg");
          break;
      case DoorStatus::DEPOSIT_ERR:
          snprintf(labelText, sizeof(labelText), "Failed: Deposit Exceeded");
      snprintf(img_source, sizeof(img_source), "S:/deposit_err.jpg");
          break;
      case DoorStatus::CODE_EXPIRE:
          snprintf(labelText, sizeof(labelText), "Failed: Code Expired");
      snprintf(img_source, sizeof(img_source), "S:/code_expire.jpg");
          break;
      case DoorStatus::USING_ANOTHER_SHELF:
          snprintf(labelText, sizeof(labelText), "Failed: Using another shelf");
      snprintf(img_source, sizeof(img_source), "S:/using_another_shelf.jpg");
          break;
      case DoorStatus::CHECKED_OUT:
          snprintf(labelText, sizeof(labelText), "Failed: Checked out");
      snprintf(img_source, sizeof(img_source), "S:/checked_out.jpg");
          break;
      default:
          snprintf(labelText, sizeof(labelText), "Unknown Status");
      snprintf(img_source, sizeof(img_source), "S:/idle.jpg");
          break;
    }

    lv_label_set_text(label, labelText);
	  lv_img_set_src(img, img_source);
	  statusChanged = false;
    Serial.println(labelText); // Debug output
}

void resetStatus(){

  if(doorStatus == DoorStatus::IDLE) return;

  unsigned long td = millis() - lastUpdate;
   if(td > (STATUS_RESET_TIME * 1000)){
      setStatus(DoorStatus::IDLE);
      Serial.print("Reset to idle");
  }
}

void setStatus(DoorStatus status) {
    // Update every 2 seconds
    switch (status) {
    case DoorStatus::IDLE:
        doorStatus = DoorStatus::IDLE;
        statusChanged = true;
        break;
    case DoorStatus::LOADING:
        doorStatus = DoorStatus::LOADING;
        statusChanged = true;
        break;
    case DoorStatus::SUCCESS:
        doorStatus = DoorStatus::SUCCESS; // Or random failure
        statusChanged = true;
        break;
    case DoorStatus::EQUIP_ERR:
        doorStatus = DoorStatus::EQUIP_ERR; // Or random failure
        statusChanged = true;
        break;
    case DoorStatus::INUSE:
        doorStatus = DoorStatus::INUSE; // Or random failure
        statusChanged = true;
        break;
    case DoorStatus::DEPOSIT_ERR:
        doorStatus = DoorStatus::DEPOSIT_ERR; // Or random failure
        statusChanged = true;
        break;
    case DoorStatus::CODE_EXPIRE:
        doorStatus = DoorStatus::CODE_EXPIRE; // Or random failure
        statusChanged = true;
        break;
    case DoorStatus::USING_ANOTHER_SHELF:
        doorStatus = DoorStatus::USING_ANOTHER_SHELF; // Or random failure
        statusChanged = true;
        break;
    case DoorStatus::CHECKED_OUT:
        doorStatus = DoorStatus::CHECKED_OUT; // Or random failure
        statusChanged = true;
        break;

    }
    lastUpdate = millis();
}

static void scroll_label_cb(lv_timer_t* timer) {
    lv_obj_t* label = (lv_obj_t*)lv_timer_get_user_data(timer);
    lv_coord_t x = lv_obj_get_x(label);
    lv_coord_t width = lv_obj_get_width(label);

    // Move the label left
    x -= 10; 

    // Reset if it goes off-screen
    if (x < -width) {
        x = LV_HOR_RES; // Move it back to the right side of the screen
    }

    // Update the label's position
    lv_obj_set_x(label, x);
}

DoorStatus convertResponseToDoorStatus(String response) {
    // Convert String to integer
    int responseCode = response.toInt();

    // Map the integer to DoorStatus
    switch (responseCode) {
        case 1:
            return DoorStatus::SUCCESS;    // "1" -> Success
        case 2:
            return DoorStatus::EQUIP_ERR;  // "2" -> Equipment error
        case 3:
            return DoorStatus::INUSE;      // "3" -> In use
        case 4:
            return DoorStatus::DEPOSIT_ERR; // "4" -> Deposit exceed
        case 5:
            return DoorStatus::CODE_EXPIRE; // "5" -> Code expired
        case 6:
            return DoorStatus::USING_ANOTHER_SHELF;
        case 7:
            return DoorStatus::CHECKED_OUT;
        default:
            return DoorStatus::IDLE;       // Non-numeric or invalid -> IDLE
    }
}

void handleQrInput(){
  while (Serial.available() > 0 && !scanComplete) {
    // Read the incoming byte
    Serial.print(scannedString);
    char incomingByte = Serial.read();
     if (incomingByte == '@') {
      scanComplete = true;  
    } 
    else 
    {
      scannedString += incomingByte;  
    }
  }

  if(scanComplete)
  {
    checkWiFiConnection();
    lv_label_set_text(label, scannedString.c_str());
    //setStatus(DoorStatus::SUCCESS);

    Serial.print("Received: ");
    Serial.println(scannedString);
    testJsonString(scannedString);
    doorStatus = convertResponseToDoorStatus(sendPostRequest(scannedString));
    setStatus(doorStatus);
    scannedString = "";
    scanComplete = false;
  }
}

void setup() {
  Serial.begin(9600);
  lv_init();
  lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

    //try to open sd card
  if (!SD.begin(5)) {
    Serial.println("SD card Failed");
    return;
  }



  //Create Logo
  img = lv_image_create(lv_screen_active());
  lv_image_set_src(img, "S:/logo.jpg"); // Path to JPEG on SD card
  lv_obj_set_size(img, 240, 60);
  lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0); // Center the image

  // Create image object
  img = lv_image_create(lv_screen_active());
  lv_image_set_src(img, "S:/idle.jpg"); // Path to JPEG on SD card
  lv_obj_set_size(img, 240, 240);
  lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 60); // Center the image

    // Create label
  label = lv_label_create(lv_screen_active());
  lv_obj_set_size(label, 240, 20);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

  // Connect to WiFi and update label
  char labelText[64]; // Buffer for dynamic text
  if (connectToWifi()) {
    snprintf(labelText, sizeof(labelText), "Successfully Connected");
    Serial.println("\nSuccess");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    snprintf(labelText, sizeof(labelText), "Failed to connect WiFi");
    Serial.println("\nFailed");
  }

  lv_label_set_text(label, labelText);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  //lv_timer_t *timer = lv_timer_create(scroll_label_cb, 100, label);

  Serial.println("Setup done");
}

void loop()
{

    lv_timer_handler(); /* let the GUI do its work */
    lv_tick_inc(5);
    delay(5); /* let this time pass */

    resetStatus();
    updateDoorStatusDisplay(label, img);
    handleQrInput();
    

}
