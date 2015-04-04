#include <usb.h>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>

#include <pthread.h>

using namespace std;
using namespace cv;

/** Function Headers */
void detectAndDisplay( Mat frame );

usb_dev_handle* open_launcher();
int send_message(char* msg, int index);
void movement_handler(usb_dev_handle*, char control);

void *slew(void*);

/** Global variables */
String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
string window_name = "Capture - Face detection";
RNG rng(12345);

pthread_t slew_thread;
pthread_mutex_t slew_mtx = PTHREAD_MUTEX_INITIALIZER;

#define DOWN    (1)
#define UP      (2)
#define LEFT    (4)
#define RIGHT   (8)
#define STOP    (32)
#define DZONE   (10)
const Size size(256, 256);
const Point screen_center(size.width / 2, size.height / 2);

Point target;

/** @function main */
int main( int argc, const char** argv )
{
        CvCapture* capture;
        int cam = 1;

        if (argc > 1)
                cam = atoi(argv[1]);

        if (pthread_create(&slew_thread, NULL, slew, NULL) != 0)
                return EXIT_FAILURE;

        //-- 1. Load the cascades
        if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
        if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };

        //-- 2. Read the video stream
        capture = cvCaptureFromCAM( cam );
        if( capture )
        {
                while( true )
                {
                        Mat frame = cvQueryFrame( capture );

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

void* slew(void*)
{
        usb_init();
        usb_dev_handle* launcher = open_launcher();
        if (!launcher)
                return (int*) EXIT_FAILURE;

        while(true) {
                int mult = 1;
                char cmd = 0;

                pthread_mutex_lock(&slew_mtx);
                int difx = abs(target.x);
                int dify = abs(target.y);
                int len = (int) sqrt(difx * difx + dify * dify);

                if (len < DZONE) {
                        cmd = STOP;
                        mult = 20;
                } else if (difx > dify) {
                        mult = 20;
                        if (target.x < 0) {
                                cmd = LEFT;
                                target.x -= 2;
                        } else if (target.x > 0) {
                                cmd = RIGHT;
                                target.x -= 2;
                        }
                } else if (dify > difx) {
                        mult = 20;
                        if (target.y > 0) {
                                cmd = DOWN;
                                target.y -= 2;
                        } else if (target.y < 0) {
                                cmd = UP;
                                target.y += 2;
                        }
                }
                pthread_mutex_unlock(&slew_mtx);

                if (cmd != 0)
                        movement_handler(launcher, cmd);

                const struct timespec delaytime = { 0, mult * 100000L };
                nanosleep(&delaytime, NULL);
        }
        return NULL;
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

void movement_handler(usb_dev_handle* launcher, char control)
{
        static char msg[8];
        msg[0] = control;
        msg[1] = 0x0;
        msg[2] = 0x0;
        msg[3] = 0x0;
        msg[4] = 0x0;
        msg[5] = 0x0;
        msg[6] = 0x0;
        msg[7] = 0x0;

        usb_control_msg(launcher, 0x21, 0x9, 0x200, 0, msg, 8, 1000);
}

/** @function detectAndDisplay */
void detectAndDisplay( Mat large )
{
        std::vector<Rect> faces;
        Mat frame;
        Mat frame_gray;

        resize(large, frame, size);
        const Size2f mult((float) large.cols / size.width, (float) large.rows / size.height);
        cvtColor( frame, frame_gray, CV_BGR2GRAY );
        equalizeHist( frame_gray, frame_gray );

        //-- Detect faces
        face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

        int least_dist2 = INT_MAX;
        Point new_target;
        for( size_t i = 0; i < faces.size(); i++ )
        {
                Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );

                Point diff = center - screen_center;
                int dist2 = diff.x * diff.x + diff.y * diff.y;
                if (dist2 < least_dist2) {
                        new_target = diff;
                        least_dist2 = dist2;
                }

                Size radii( faces[i].width*0.5, faces[i].height*0.5);
                center.x *= mult.width;
                center.y *= mult.height;
                radii.width *= mult.width;
                radii.height *= mult.height;
                ellipse( large, center, radii, 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

                Mat faceROI = frame_gray( faces[i] );
                std::vector<Rect> eyes;

                //-- In each face, detect eyes
                eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );

                for( size_t j = 0; j < eyes.size(); j++ )
                {
                        Point center( faces[i].x + eyes[j].x + eyes[j].width*0.5, faces[i].y + eyes[j].y + eyes[j].height*0.5 );
                        center.x *= mult.width;
                        center.y *= mult.height;
                        int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
                        circle( large, center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
                }
        }

        if (sqrt(least_dist2) >= DZONE) {
                if (least_dist2 != INT_MAX)
                        printf ("Found target at (%d, %d)! range: %lf\n", new_target.x, new_target.y, sqrt(least_dist2));
                pthread_mutex_lock(&slew_mtx);
                target = new_target;
                pthread_mutex_unlock(&slew_mtx);
        }

        //-- Show what you got
        imshow( window_name, large );
}
