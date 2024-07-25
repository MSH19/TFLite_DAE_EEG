# TFLite_DAE_EEG    

Running deep learning auto encoder on Arduino Nano 33 BLE    

![alt text](images/arduino_nano_33.png)         

TensorflowLite library should be added to the Arduino IDE (library Zip file is included in library folder) or can be downloaded from: https://github.com/tensorflow/tflite-micro-arduino-examples     

The Autoencoder_revision folder contains the original trained model on PC        

To load the original model and to convert it to tflite models (float and int8), use the script named      

To convert the model to TFLite models, use the script named tflite_conversion. It also loads the noisy and clean data and exports them to text files (optional).

To run the model on the arduino Nano 33 BLE device, use the program named arduino_dae_eeg         
