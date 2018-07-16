// Serial communication
#include <fcntl.h>      /* File control definitions */
#include <errno.h>      /* Error number definitions */
#include <termios.h>    /* POSIX terminal control definitions */


// ================================================================================================
// Serial communication between two computers
// ================================================================================================
void SerialTalk(void)
{
    // ---------------------------------------------
    // Open serial port
    // ---------------------------------------------
    int fd;
    fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);    //0_RDWR:   Read/Write mode
                                                                //0_NOCTTY: program doesn't want to be "controlling terminal" for port
                                                                //0_NDELAY: program doesn't care what state the DCD signal line is in

    // Check to see if serial port is opened
    if (fd == -1) {
        // Could not open the port
        perror("open_port: Unable to open /dev/ttyUSB0 - ");
    }
    else
        fcntl(fd, F_SETFL, 0);

    printf("File descriptor: %d\r\n",fd);


    // ---------------------------------------------
    // Configure serial port
    // ---------------------------------------------
    struct termios options;

    // Get current attributes
    tcgetattr(fd, &options);

    // Canonical input - carriage return or line feed
    options.c_lflag |= (ICANON | ECHO | ECHOE);

    // 230400 baud rate
    cfsetispeed(&options, B230400);
    cfsetospeed(&options, B230400);

    // 8N1: 8 data bits, no parity, 1 stop bit
    options.c_cflag &= ~PARENB
    options.c_cflag &= ~CSTOPB
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // Hardware flow control
    //options.c_cflag |= CNEW_RTSCTS;

    // Software flow control
    options.c_iflag |= (IXON | IXOFF | IXANY);

    // Input parity checking
    options.c_iflag |= (INPCK | ISTRIP);

    // Processed output
    options.c_oflag |= OPOST;

    //Enable the receiver and set local mode
    options.c_cflag |= (CLOCAL | CREAD);

    //Set the new options for the port immediately
    tcsetattr(fd, TCSANOW, &options);


    // ---------------------------------------------
    // Write to serial port
    // ---------------------------------------------
    n = write(fd, "ATZ\r", 4);  //4 bytes
    if (n < 0)
        fputs("write() of 4 bytes failed!\n", stderr);


    // ---------------------------------------------
    // Read from serial port
    // ---------------------------------------------
    fcntl(fd, F_SETFL, 0);


    // ---------------------------------------------
    // Close serial port
    // ---------------------------------------------
    close(fd);
}
