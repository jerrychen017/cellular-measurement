# Cellular Measurement Tools
UDP based applications written in C to do bidirectional bandwidth/latency measurements between cellular and server. It also includes 
a server to manage packets when the interactive application is running.
You can find the cellular measurement Android application [here][https://github.com/jerrychen017/cellular-measurement-android.git].

## Functionalities:
#### Bidirectional Measurement Application
* Application spawns sender and receiver threads
* Each sender consists of a controller and a data generator
* Data generator is responsible for creating data at a bitrate and the controller is responsible for doing packet bursting and adjusting the sending bitrate
* Receiver is able to estimate bandwidth with two different methodologies Running Average and EWMA
* Grace Period is the number of times that we allow bandwidth estimations on the receiver to drop below our set threshold before we need to act on it

#### Interactive Application
* Works in conjunction with an Android application that sends coordinates to the interactive server
* The server echoes back the tapped coordinates on the Android application canvas
* The Android application will then draw circles based on the send back coordinates.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details