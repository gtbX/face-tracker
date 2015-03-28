#include <usb.h>

 #include "opencv2/objdetect/objdetect.hpp"
 #include "opencv2/highgui/highgui.hpp"
 #include "opencv2/imgproc/imgproc.hpp"

 #include <iostream>
 #include <stdio.h>

 using namespace std;
 using namespace cv;

 /** Function Headers */
 void detectAndDisplay( Mat frame );

 usb_dev_handle* open_launcher();
 int send_message(char* msg, int index);
 void movement_handler(char control);

 /** Global variables */
 String face_cascade_name = "haarcascade_frontalface_alt.xml";
 String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
 CascadeClassifier face_cascade;
 CascadeClassifier eyes_cascade;
 string window_name = "Capture - Face detection";
 RNG rng(12345);

 usb_dev_handle* launcher;

 const Size size(256, 256);
 const Point sc(128, 128);

 /** @function main */
 int main( int argc, const char** argv )
 {
   CvCapture* capture;
   Mat frame;

   usb_init();
   launcher = open_launcher();
   if (!launcher)
     return EXIT_FAILURE;

   //-- 1. Load the cascades
   if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
   if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };

   //-- 2. Read the video stream
   capture = cvCaptureFromCAM( 1 ); // /dev/video1
   if( capture )
   {
     while( true )
     {
   Mat large = cvQueryFrame( capture );
   resize(large, frame, size);

   //-- 3. Apply the classifier to the frame
       if( !frame.empty() )
       { detectAndDisplay( frame ); }
       else
       { printf(" --(!) No captured frame -- Break!"); break; }

       int c = waitKey(10);
       if( (char)c == 'c' ) { break; }
      }
   }
   return 0;
 }

usb_dev_handle* open_launcher()
{
    usb_find_busses();
    usb_find_devices();

    for (struct usb_bus *bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (struct usb_device *dev = bus->devices; dev; dev = dev->next)
        {
            if (dev->descriptor.idVendor == 0x0a81)
            {
                usb_dev_handle *l = usb_open(dev);
                int claimed = usb_claim_interface(l, 0);
                if (claimed == 0)
                    return l;
                else
                    usb_close(l);
            }
        }
    }

    printf ("No launcher found\n"); 
    return NULL;
}

//wrapper for control_msg
int send_message(char* msg, int index)
{
	int i = 0;
	int j = usb_control_msg(launcher, 0x21, 0x9, 0x200, index, msg, 8, 1000);

	//be sure that msg is all zeroes again
	msg[0] = 0x0;
	msg[1] = 0x0;
	msg[2] = 0x0;
	msg[3] = 0x0;
	msg[4] = 0x0;
	msg[5] = 0x0;
	msg[6] = 0x0;
	msg[7] = 0x0;

	return j;
}

void movement_handler(char control)
{
	char msg[8];
	//reset
	msg[0] = 0x0;
	msg[1] = 0x0;
	msg[2] = 0x0;
	msg[3] = 0x0;
	msg[4] = 0x0;
	msg[5] = 0x0;
	msg[6] = 0x0;
	msg[7] = 0x0;

	//send 0s
	int deally = send_message(msg, 1);

	//send control
	msg[0] = control;
	deally = send_message(msg, 0);

	//and more zeroes
	deally = send_message(msg, 1);
				
}

/** @function detectAndDisplay */
void detectAndDisplay( Mat frame )
{
  std::vector<Rect> faces;
  Mat frame_gray;

  cvtColor( frame, frame_gray, CV_BGR2GRAY );
  equalizeHist( frame_gray, frame_gray );

  //-- Detect faces
  face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

  int least_dist2 = INT_MAX;
  Point target;
  for( size_t i = 0; i < faces.size(); i++ )
  {
    Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
    ellipse( frame, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

    Point diff = center - sc;
    int dist2 = diff.x * diff.x + diff.y * diff.y;
    if (dist2 < least_dist2) {
      target = diff;
      least_dist2 = dist2;
    }

    Mat faceROI = frame_gray( faces[i] );
    std::vector<Rect> eyes;

    //-- In each face, detect eyes
    eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );

    for( size_t j = 0; j < eyes.size(); j++ )
     {
       Point center( faces[i].x + eyes[j].x + eyes[j].width*0.5, faces[i].y + eyes[j].y + eyes[j].height*0.5 );
       int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
       circle( frame, center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
     }
  }
  printf ("Found target at (%d, %d)!\n", target.x, target.y);

  // slew into position
  int difx = abs(target.x);
  int dify = abs(target.y);
  if (difx > dify) {
    if (target.x < 0)
      movement_handler(4);
    else if (target.x > 0)
      movement_handler(8);
  } else if (dify > difx) {
    if (target.y > 0)
      movement_handler(1);
    else if (target.y < 0)
      movement_handler(2);
  } else
    movement_handler(32);

  //-- Show what you got
  imshow( window_name, frame );
 }
