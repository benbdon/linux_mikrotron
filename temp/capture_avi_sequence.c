/*
 *  INSTRUCTIONS:
 *
 *  1)	Define FormatFile to be loaded when initializing frame grabber.
 *
 */
#if !defined(FORMAT) && !defined(FORMATFILE)
  #define FORMATFILE	"ExTrigger_1024_150_0_05ms.fmt"	  // using format file saved by XCAP
#endif


/*
 *  2) Set number of expected PIXCI(R) image boards.
 *  This example expects only one unit.
 */
#if !defined(UNITS)
    #define UNITS	1
#endif
#define UNITSMAP    ((1<<UNITS)-1)  // shorthand - bitmap of all units


/*
 *  3) Optionally, set driver configuration parameters.
 *  These are normally left to the default, "".
 *
 *  Note: Under Linux, the image frame buffer memory can't be set as
 *  a run time option. It MUST be set via insmod so the memory can
 *  be reserved during Linux's initialization.
 */
#if !defined(DRIVERPARMS)
  //#define DRIVERPARMS "-QU 0"       // don't use interrupts
    #define DRIVERPARMS ""	      // default
#endif


/*
 *  4)	Choose whether the optional PXIPL Image Processing Library
 *	is available.
 */
#if !defined(USE_PXIPL)
    #define USE_PXIPL	0
#endif


/*
 *  5a) Compile with GCC w/out PXIPL for 64 bit Linux as:
 *
 *	    gcc `pkg-config --cflags opencv` `pkg-config --libs opencv` -DC_GNU64=400 -DOS_LINUX -I../.. capture_avi_sequence.c ../../xclib_x86_64.a -lm
 *
 *	Compile with GCC with PXIPL (do not have at this time) for 64 bit Linux as :
 *
 *	    gcc -DC_GNU64=400 -DOS_LINUX -I../.. triggertest1.c ../../pxipl_x86_64.a ../../xclib_x86_64.a -lm
 *
 *
 *  5b) Run the output file from GCC (must be super-user or sudo permission):
 *
 *	    ./a.out
 *
 */




// XCLIB Reference Manual -> /usr/local/xclib3_8/xclib.htm






// ================================================================================================
// NECESSARY INCLUDES: 
// ================================================================================================

// C library
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// UNIX standard function definitions
#include <unistd.h> 

// Function prototypes for Simple C Programming (SCP)
#include "xcliball.h"

#if USE_PXIPL
  #include "pxipl.h"
#endif

// OpenCV2
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// UDP communication
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVERA "129.105.69.140"    // server IP address (Machine A - Windows & TrackCam)
#define PORTA    9090               // port on which to send data (Machine A)
#define SERVERB "129.105.69.220"    // server IP address (Machine B - Linux & Mikrotron)
#define PORTB    51717              // port on which to listen for incoming data (Machine B)
#define BUFLEN   512                // max length of buffer







// Create UDP socket structures for Machine A and Machine B
struct sockaddr_in AddrMachineA, AddrMachineB;




// ================================================================================================
// SUPPORT STUFF:
// Catch CTRL+C and floating point exceptions so that once opened, the PIXCI(R) driver
// and frame grabber are always closed before exit
// ================================================================================================
void sigintfunc(int sig)
{
    pxd_PIXCIclose();
    exit(1);
}



// ================================================================================================
// Video 'interrupt' callback function 
// ================================================================================================
int fieldirqcount = 0;
void videoirqfunc(int sig)
{
    fieldirqcount++;
}



// ================================================================================================
// Socket error
// ================================================================================================
void die(char *sock)
{
    perror(sock);
    exit(1);
}



// ================================================================================================
// Create UDP socket, set up Machine A structure, bind socket to port for Machine B
// ================================================================================================
int InitializeUDP(int sock)
{
    // Create a UDP socket
    if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    printf("Socket created.\r\n");


    // Zero out the structure for Machine A
    memset((char *) &AddrMachineA, 0, sizeof(AddrMachineA));

    AddrMachineA.sin_family = AF_INET;
    AddrMachineA.sin_port = htons(PORTA);

    // IP address of the receiver
    if (inet_aton(SERVERA, &AddrMachineA.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    printf("Machine A structure set up with IP address of the receiever and the specified port.\r\n");


    // Zero out the structure for Machine B
    memset((char *) &AddrMachineB, 0, sizeof(AddrMachineB));

    AddrMachineB.sin_family = AF_INET;
    AddrMachineB.sin_port = htons(PORTB);
    AddrMachineB.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind UDP socket to port for Machine B
    if( bind(sock, (struct sockaddr*)&AddrMachineB, sizeof(AddrMachineB) ) == -1 )
    {
        die("bind");
    }
    printf("UDP socket binded to any address and the specified port.\r\n\n");

    return sock;
}



// ================================================================================================
// Receive message from Machine A
// ================================================================================================
void ReceiveSocket(int sock, char buf[], int slen)
{
    // Try to receive some data from Machine A (blocking call)
    printf("---------------------------------------------------------------------------------------\r\n\n");
    printf("Waiting for data...\r\n");
    fflush(stdout);

    int recv_len;
    if( (recv_len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &AddrMachineA, &slen)) == -1 )
    {
        die("recvfrom()");
    }
}



// ================================================================================================
// Send message to Machine A
// ================================================================================================
void SendSocket(int sock, char message[], int slen)
{
    // Send the message to Machine A
    if( sendto(sock, message, strlen(message) , 0 , (struct sockaddr *) &AddrMachineA, slen) == -1 )
    {
        die("sendto()");
    }
    printf("Message sent to Machine A.\r\n\n");
}



// ================================================================================================
// Close UDP socket
// ================================================================================================
void CloseSocket(int sock)
{
    // Close socket
    close(sock);
    printf("Closed socket.\r\n");
}



// ================================================================================================
// Open and initialize frame grabber
// ================================================================================================
int InitializationFrameGrabber(void)
{
    // Open the XCLIB C Library for use
    int i;

    printf("Opening EPIX(R) PIXCI(R) Frame Grabber, ");

    // Either FORMAT or FORMATFILE should have been selected above
    #if defined(FORMAT)
	printf("using predefined format '%s'.\n", FORMAT);
	i = pxd_PIXCIopen(DRIVERPARMS, FORMAT, "");
    #elif defined(FORMATFILE)
	printf("using format file '%s'.\n", FORMATFILE);
	i = pxd_PIXCIopen(DRIVERPARMS, "", FORMATFILE);
    #endif

    // Open Error
    if (i < 0) {
	printf("Open Error %d\a\a\n", i);
	pxd_mesgFault(UNITSMAP);
	return(i);
    }

    printf("Open Okay.\r\n");


    // Report image resolution
    printf("Image resolution: %d X %d\r\n\n", pxd_imageXdim(), pxd_imageYdim());

    return(0);
}



// ================================================================================================
// Capture sequence AVI
// ================================================================================================
void CaptureSequenceAVI(int NUMIMAGES, int FPS, int PULSETIME, float DELAYTIME, int HORIZ_AMPL, int VERT_AMPL, int FREQ, int PHASE_OFFSET, char IDENTIFIER, int SAVEDSIGNAL, int sock)
{

    // Initialize last buffer 
    pxbuffer_t lastbuf=0;

    // Allocate memory for buf array to store pointers to images
    unsigned char* buf[30000];
    int i;
    for(i=0; i<30000; i++)
    {
        buf[i] = (unsigned char*)malloc( pxd_imageXdim()*pxd_imageYdim()*sizeof(unsigned char) );
    }

    // Initialize tempbuf for live image processing and allocate memory for one frame
    unsigned char* tempbuf;
    tempbuf = (unsigned char*) malloc( pxd_imageXdim()*pxd_imageYdim()*sizeof(unsigned char) );


    // Send UDP message to Machine A to start sequence AVI - otherwise Machine A starts before Machine B is ready to capture
    int slen=sizeof(AddrMachineA);
    char message[BUFLEN];

    AddrMachineA.sin_port = htons(PORTA);    // specify PortA to send message back to
    strcpy(message, "Start sequence AVI.");
    SendSocket(sock, message, slen);


    // For a camera in asynchronous trigger mode, with an external trigger, sequence capture is simply:
    printf("Ready to capture sequence AVI.\r\n\n");
    // The pxd_goLiveSeq initiates sequence capture of images into startbuf through endbuf.
    // The sequence capture starts into startbuf and continues into frame buffers startbuf+incbuf*1, startbuf+incbuf*2, etc., 
    // wrapping around from the endbuf back to the startbuf.
    pxd_goLiveSeq(UNITSMAP,         // select PIXCI(R) unit 1
                  1,                // select start frame buffer
                  (NUMIMAGES-1),    // select last frame buffer
                  1,                // incrementing by one buffer
                  (NUMIMAGES-1),    // for this many captures
                  1);               // advancing to next buffer after each 1 frame


/*
    // Infinite for loop
    for (;;) {

        // If a new buffer was not yet captured, continue with the for(;;) loop
        if (pxd_capturedBuffer(1) == lastbuf) {

            // Might be able to solve the blocking function call problem when frame grabber misses frames stalling the program
            if(lastbuf > 10) {

                // Have some counter in here that would call pxd_goAbortLive() if the time after 
                // current frame but before the next frame is much greater than 1/FPS_Side

            }

            // Skips the remaining code in the for loop (will start over at the top)
            continue;
        }

        // Update lastbuf with new buffer
        lastbuf = pxd_capturedBuffer(1);
        printf("Frame number captured: %d\r\n", lastbuf);



// UNTESTED - For live video analysis
// Once a new frame has filled the buffer, convert the frame buffer to OpenCV IplImage* (array)
// With the IplImage*, threshold the array, perform cvHoughCircles to locate the drop
// If more than one drop, either threshold with better parameters or try adding other filters (GaussianBlur, etc.)
// If necessary, can also terminate the pxd_goLiveSeq() call [i.e., drop coalesce and want to restart]


        //Store pointer to lastbuf frame in tempbuf
        pxd_readuchar(UNITSMAP, lastbuf, 0, 0, -1, -1, tempbuf, pxd_imageXdim()*pxd_imageYdim()*sizeof(unsigned char), "Grey");


        // Create IplImage* TempImg in order to write the frame buffers to avi object
        IplImage* TempImg33;
        TempImg33 = cvCreateImage(cvSize(pxd_imageXdim(),pxd_imageYdim()), IPL_DEPTH_8U, 1);

        // CvScalar tuple for setting pixel values from frame buffer
        CvScalar s;

        // Reads each image from respective buffers in memory, storing as IplImage* (necessary for cvWriteFrame)
        int l, m, counter=0;
        for(l=0; l < pxd_imageYdim(); l++){
            for (m=0; m < pxd_imageXdim(); m++) {
                s.val[0] = tempbuf[counter];
                cvSet2D(TempImg33, l, m, s); // set the (l,m) pixel value using s.val[0]
                counter++;
            }
        }


        // Threshold TempImg33
        // http://docs.opencv.org/2.4/modules/imgproc/doc/miscellaneous_transformations.html?highlight=threshold#threshold
        IplImage* TempImg33_Thres;
        TempImg33_Thres = cvCreateImage(cvSize(pxd_imageXdim(),pxd_imageYdim()), IPL_DEPTH_8U, 1);

        double threshold=220;  //threshold value
        double max_value=255;  //maximum value to use with the THRESH_BINARY and THRESH_BINARY_INV thresholding types.
        int threshold_type=THRESH_BINARY;  //thresholding type

        cvThreshold( TempImg33, TempImg33_Thres, threshold, max-value, threshold_type );

        //cvNamedWindow("thres", 1);
        //cvShowImage("thres", TempImg33_Thres);
        //cvWaitKey(0);


        // Locate circles in TempImg33_Thres using cvHoughCircles
        // http://docs.opencv.org/2.4/modules/imgproc/doc/feature_detection.html?highlight=houghcircles#houghcircles
        CvMemStorage* storage = cvCreateMemStorage(0);  //memory storage that will contain the output sequence of found circles
        double dp=4;                                    //inverse ratio of the accumulator resolution to the image resolution
        double min_dist=TempImg33->width/3;             //minimum distance between the centers of the detected circles
        double param1=100;                              //higher threshold of the two passed to the Canny() edge detector
        double param2=100;                              //accumulator threshold for the circle centers at the detection stage
        int min_radius=0;                               //minimum circle radius
        int max_radius=0;                               //maximum circle radius

        CvSeq* results = cvHoughCircles( TempImg33_Thres, storage, CV_HOUGH_GRADIENT, dp, min_dist, param1, param2, min_radius, max_radius );

        std::cout << "Number of circles detected: " << results->total << std::endl;


        // Draw detected circles on TempImg33
        for( int i=0; i<results->total; i++ ) 
        {
            float* p = (float*) cvGetSeqElem( results, i );
            CvPoint pt = cvPoint( cvRound( p[0] ), cvRound( p[1] ) );
            cvCircle( TempImg33, pt, cvRound( p[2] ), CV_RGB(0xff,0,0) );
        }

        cvNamedWindow("HoughCircles", 1);
        cvShowImage("HoughCircles", TempImg33);
        cvWaitKey(0);


        // Analysis of detected circles





        // Check if video capture has ceased, if so, break from the infinite for loop
        if (pxd_goneLive(UNITSMAP, 0)==0) {
            break;
        }

        // Either of these functions can be used to terminate the pxd_goLiveSeq().
        //int pxd_goUnLive(unitmap);     // terminates pxd_goLiveSeq after the current field or frame
        //int pxd_goAbortLive(unitmap);  // terminates pxd_goLiveSeq immediately, even if in the middle of a field, line, or pixel
    }
*/


    int actualFrameCounter = 0;

    // Infinite for loop
    for (;;) {

        // If a new buffer was not yet captured, continue with the for(;;) loop
        if (pxd_capturedBuffer(1) == lastbuf) {
            continue; // skips the remaining code in the for loop (will start over at the top)
        }

        // Update lastbuf with new buffer
        lastbuf = pxd_capturedBuffer(1);
        actualFrameCounter = actualFrameCounter + 1;
        printf("Frame number captured: %d\r\n", lastbuf);

        // Check if video capture has ceased, if so, break from the infinite for loop
        if (pxd_goneLive(UNITSMAP, 0)==0) {
            break;
        }
    }

    printf("\r\nTotal # of frames captured: %d/%d\r\n", actualFrameCounter, (NUMIMAGES-1) );



    // Wait for capture to cease
    while (pxd_goneLive(UNITSMAP, 0)) {  // The pxd_goneLive returns 0 if video capture is not currently in effect. 
        ;                                // Otherwise, a non-zero value is returned. 
    }
    printf("Sequence AVI captured.\r\n");


    // Store pointers to images in buf[]
    int j;
    for(j=0; j<(NUMIMAGES-1); j++)
    {
        //j+1th frame -> buf[j]
        pxd_readuchar(UNITSMAP, j+1, 0, 0, -1, -1, buf[j], pxd_imageXdim()*pxd_imageYdim()*sizeof(unsigned char), "Grey"); 
    }
    printf("Pointers to frame buffers stored in buf[].\r\n\n");
    

/*
    // Save BMP image using XCLIB
    char *name1 = "/usr/local/xclib3_8/examples/C/image1.bmp";
    pxd_saveBmp(UNITSMAP, name1, 1, 0, 0, -1, -1, 0, 0);    // 3rd argument is frame buffer

    // Reads an image from a buffer in memory.
    IplImage* TempImgMat;
    TempImgMat = cvCreateImage(cvSize(pxd_imageXdim(),pxd_imageYdim()), IPL_DEPTH_8U, 1);
    int i, j, counter=0;
    CvScalar s;
    for(i=0; i < pxd_imageYdim(); i++){
        for (j=0; j < pxd_imageXdim(); j++) {
            s.val[0] = buf[counter];
            cvSet2D(TempImgMat, i, j, s); // set the (i,j) pixel value
            counter++;
        }
    }

    // Saves an image from image buffer to a specified file.
    const char* filename3 = "/usr/local/xclib3_8/examples/C/image1_from_buffer.bmp";
    const int* params=0;
    cvSaveImage(filename3, TempImgMat, params);
    printf("Image1 from buffer -> saved.\r\n");
*/

    // Create VideoWriter using Huffyuv encoding at 5 fps - save to MacIver->Documents->High Speed Videos
    char filename[64];
    int fourcc = CV_FOURCC('H','F','Y','U');
    double fps = 5;
    CvSize frame_size;
    frame_size = cvSize( pxd_imageXdim(), pxd_imageYdim() );
    int is_color = 0;

    if (IDENTIFIER == 'S') {
        sprintf(filename, "/home/maciver/Documents/High Speed Videos/Mikrotron_%c_%d_%dHz_%fDelayTime_%dFPS_%dPulseTime.avi", IDENTIFIER, SAVEDSIGNAL, FREQ, DELAYTIME, FPS, PULSETIME);
    }
    else if (IDENTIFIER == 'E') {
        sprintf(filename, "/home/maciver/Documents/High Speed Videos/Mikrotron_%c_%dHz_%dA_%dA_%03dDPhase_%fDelayTime_%dFPS_%dPulseTime.avi", IDENTIFIER, FREQ, HORIZ_AMPL, VERT_AMPL, PHASE_OFFSET, DELAYTIME, FPS, PULSETIME);
    }

    CvVideoWriter* writer;
    writer = cvCreateVideoWriter(filename, fourcc, fps, frame_size, is_color);
    printf("VideoWriter created.\r\n");


    // Loop through each frame, writing them to the avi file
    printf("Starting to write frames to AVI file.\r\n");
    int k;
    for(k=0; k<(NUMIMAGES-1); k++)
    {
        // Create IplImage* TempImg in order to write the frame buffers to avi object
        IplImage* TempImg;
        TempImg = cvCreateImage(cvSize(pxd_imageXdim(),pxd_imageYdim()), IPL_DEPTH_8U, 1);

        // CvScalar tuple for setting pixel values from frame buffer
        CvScalar s;

        // Reads each image from respective buffers in memory, storing as IplImage* (necessary for cvWriteFrame)
        int l, m, counter=0;
        for(l=0; l < pxd_imageYdim(); l++){
            for (m=0; m < pxd_imageXdim(); m++) {
                s.val[0] = buf[k][counter];
                cvSet2D(TempImg, l, m, s); // set the (l,m) pixel value using s.val[0]
                counter++;
            }
        }

        // Write frame to avi object
        cvWriteFrame(writer, TempImg);
    }


    // Release VideoWriter
    cvReleaseVideoWriter(&writer);



    // Check for faults, such as erratic sync or insufficient PCI bus bandwidth
    pxd_mesgFault(UNITSMAP);
    printf("AVI file written.\r\n\n");
}



// ================================================================================================
// Close the PIXCI(R) frame grabber
// ================================================================================================
void CloseFrameGrabber(void)
{
    pxd_PIXCIclose();
    printf("PIXCI(R) frame grabber closed.\r\n\n\n");
}








// ================================================================================================
// Main function
// ================================================================================================
main(void)
{
    // Catch signals
    signal(SIGINT, sigintfunc);
    signal(SIGFPE, sigintfunc);


    // Local variables used for UDP communication
    int sock, slen=sizeof(AddrMachineA);
    char buf[BUFLEN];
    char message[BUFLEN];


    // Local variables used for checking to see if still running tests
    int Run_Flag = 1;


    // Local variables used for capturing sequence AVI
    int FREQ=0, VERT_AMPL=0, HORIZ_AMPL=0, PHASE_OFFSET=0, FPS_Side=0, NUMIMAGES_Side=0, PULSETIME=0, currentSample=0, SAVEDSIGNAL=0;
    float DELAYTIME=0;
    char IDENTIFIER=0, FirstChar=0;

    int statusFrameGrabber;


    // Initialize UDP socket
    sock = InitializeUDP(sock);



    // Continue to capture sequence AVI's while Run_Flag is ON (1)
    while( Run_Flag ) {

        // Receive message from Machine A - [FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME]
        ReceiveSocket(sock, buf, slen);

        printf("Received packet from %s: %d\n", inet_ntoa(AddrMachineA.sin_addr), ntohs(AddrMachineA.sin_port));

        sscanf(buf, "%c", &FirstChar);

        if(FirstChar == 'S') {
            sscanf(buf, "%c%*c %d%*c %d%*c %d%*c %d%*c %f%*c %d", &IDENTIFIER, &SAVEDSIGNAL, &FREQ, &FPS_Side, &NUMIMAGES_Side, &PULSETIME, &DELAYTIME);
            printf("IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME: %c %d %d %d %d %f %d\r\n",IDENTIFIER, SAVEDSIGNAL, FREQ, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME);
        }
        else if(FirstChar == 'E') {
            sscanf(buf, "%c%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c %f", &IDENTIFIER, &FREQ, &VERT_AMPL, &HORIZ_AMPL, &PHASE_OFFSET, &FPS_Side, &NUMIMAGES_Side, &PULSETIME, &DELAYTIME);
            printf("IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME: %c %d %d %d %d %d %d %d %f\r\n",IDENTIFIER, FREQ, VERT_AMPL, HORIZ_AMPL, PHASE_OFFSET, FPS_Side, NUMIMAGES_Side, PULSETIME, DELAYTIME);
        }


        // Open and initialize frame grabber
        statusFrameGrabber = InitializationFrameGrabber();

        // Try opening again if did not work the first time - happens every once in a while
        if (statusFrameGrabber < 0) {
            printf("\nTrying to open frame grabber one more time.\r\n\n");
            statusFrameGrabber = InitializationFrameGrabber();
        }

        // If did not work after the second time, close the program
        if (statusFrameGrabber < 0) {
            printf("\nFrame grabber did not open after two attempts -- closing program.\r\n");
            return(1);
        }


        // Capture sequence AVI
        CaptureSequenceAVI(NUMIMAGES_Side, FPS_Side, PULSETIME, DELAYTIME, HORIZ_AMPL, VERT_AMPL, FREQ, PHASE_OFFSET, IDENTIFIER, SAVEDSIGNAL, sock);

        // Close frame grabber
        CloseFrameGrabber();


        // Check to see if still running tests from Machine A
        ReceiveSocket(sock, buf, slen);
        sscanf(buf, "%d", &Run_Flag);
        printf("Run_Flag: %d\r\n\n", Run_Flag);


        // Send UDP message to Machine A to notify receival
        memset(&message[0], 0, sizeof(message));

        AddrMachineA.sin_port = htons(PORTA);    // specify PortA to send message back to
        strcpy(message, "Message received.");
        SendSocket(sock, message, slen);        
    }


    // Close UDP socket
    CloseSocket(sock);


    return(0);
}
