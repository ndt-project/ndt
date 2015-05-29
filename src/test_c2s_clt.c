/**
 * This file contains the functions needed to handle C2S throughput
 * test (client part).
 *
 * Jakub Sławiński 2006-08-02
 * jeremian@poczta.fm
 */

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clt_tests.h"
#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "utils.h"
#include "jsonutils.h"

int pkts, lth;
int sndqueue;
double spdout, c2sspd;

typedef struct c2sWriteWorkerArgs {
  int connectionId;
  int socketDescriptor;
  double stopTime;
  char* buff;
} C2SWriteWorkerArgs;

void* c2sWriteWorker(void* arg);

/**
 * Client to server throughput test. This test performs testDuration seconds
 * memory-to-memory data transfer to test achievable network bandwidth.
 *
 * @param ctlSocket Socket on which messages are received from the server
 * @param tests     Character indicator for test to be run
 * @param host		Server name string
 * @param conn_options Options to use while connecting to server(for ex, IPV4)
 * @param buf_size  TCP send/receive buffer size
 * @param uThroughputSnapshots Variable used to set c2s throughput snapshots
 * @param jsonSupport Indicates if messages should be send using JSON format
 * @return integer > 0 if successful, < 0 in case of error
 * 		Return codes:
 * 		1: Error receiving protocol message
 *		2: Unexpected protocol message (type) received
 *		3: Improper message payload
 *		4: Incorrect message data received
 *    -1: Unable to create worker threads
 *		-3: Unable to resolve server address
 *		-11: Cannot connect to server
 *
 */
int test_c2s_clt(int ctlSocket, char tests, char* host, int conn_options,
                 int buf_size, struct throughputSnapshot **uThroughputSnapshots, int jsonSupport) {
  /* char buff[BUFFSIZE+1]; */
  char buff[64 * KILO_BITS];  // message payload.
  // note that there is a size variation between server and CLT - do not
  // know why this was changed from BUFFZISE, though
  // no specific problem is seen due to this
  char* jsonMsgValue; // temporary buffer for storing values from JSON message

  int msgLen, msgType;  // message related data
  int c2sport = atoi(PORT2);  // default C2S port
  I2Addr sec_addr = NULL;  // server address
  I2Addr sec_addresses[MAX_STREAMS];  // server addresses per thread
  int retcode;  // return code
  int one = 1;  // socket option store
  int i, k;  // temporary iterator
  C2SWriteWorkerArgs writeWorkerArgs[MAX_STREAMS]; // write workers parameters
  pthread_t writeWorkerIds[MAX_STREAMS];        // write workers ids
  double t, stop_time;  // test-time indicators
  double testDuration = 10; // default test duration
  char* strtokptr;  // pointer used by the strtok method
  struct throughputSnapshot *lastThroughputSnapshot;
  int throughputsnaps = 0; // enable the throughput snapshots writing
  int snapsdelay = 5000;   // specify the delay in the throughput snapshots thread
  int snapsoffset = 1000;  // specify the initial offset in the throughput snapshots thread
  int threadsnum = 1;      // specify the number of threads (parallel TCP connections)
  // variables used for protocol validation logs
  enum TEST_STATUS_INT teststatuses = TEST_NOT_STARTED;
  enum TEST_ID testids = C2S;


  if (tests & TEST_C2S) {  // C2S test has to be performed
    struct sigaction new, old;
    log_println(1, " <-- C2S throughput test -->");
    setCurrentTest(TEST_C2S);
    // protocol logs
    teststatuses = TEST_STARTED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);

    msgLen = sizeof(buff);

    // Initially, the server sends a TEST_PREPARE message. Any other message
    // type, or a message with payload length <0 is unexpected and
    // indicative of an error
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed prepare message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_PREPARE, msgType, buff,
                       msgLen)) {
      return 2;
    }
    buff[msgLen] = 0;
    if (jsonSupport) {
      jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
      strlcpy(buff, jsonMsgValue, sizeof(buff));
      msgLen = strlen(buff);
      free(jsonMsgValue);
    }
    if (msgLen <= 0) {
      log_println(0, "Improper message");
      return 3;
    }

    // Server sends port number to bind to in the TEST_PREPARE. Check if this
    // message body (string) is a valid integral port number.
    if (tests & TEST_EXT) {
      strtokptr = strtok(buff, " ");
      c2sport = atoi(strtokptr);
      strtokptr = strtok(NULL, " ");
      testDuration = atoi(strtokptr) / 1000.0;
      strtokptr = strtok(NULL, " ");
      throughputsnaps = atoi(strtokptr);
      strtokptr = strtok(NULL, " ");
      snapsdelay = atoi(strtokptr);
      strtokptr = strtok(NULL, " ");
      snapsoffset = atoi(strtokptr);
      strtokptr = strtok(NULL, " ");
      threadsnum = atoi(strtokptr);

      log_println(1, "  -- test duration: %.1fs", testDuration);
      log_println(1, "  -- throughput snapshots: enabled = %s, delay = %d, offset = %d",
                           throughputsnaps ? "true" : "false", snapsdelay, snapsoffset);
      log_println(1, "  -- threads: %d", threadsnum);
      lastThroughputSnapshot = NULL;
    }
    else {
      if (check_int(buff, &c2sport)) {
        log_println(0, "Invalid port number");
        return 4;
      }
    }

    log_println(1, "  -- port: %d", c2sport);

    // make struct of "address details" of the server using the host name
    if ((sec_addr = I2AddrByNode(get_errhandle(), host)) == NULL) {
      log_println(0, "Unable to resolve server address: %s", strerror(errno));
      return -3;
    }
    I2AddrSetPort(sec_addr, c2sport);  // set port value

    for (i = 0; i < threadsnum; ++i) {
      sec_addresses[i] = I2AddrCopy(sec_addr);

      if ((retcode = CreateConnectSocket(&(writeWorkerArgs[i].socketDescriptor), NULL, sec_addresses[i], conn_options, buf_size))) {
        log_println(0, "Connect() for client to server failed (connection %d)", strerror(errno), i+1);
        return -11;
      }
      setsockopt(writeWorkerArgs[i].socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }

    // Expect a TEST_START message from server now. Any other message
    // type is an error
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed start message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_START, msgType, buff,
                       msgLen)) {
      return 2;
    }

    printf("running %.1fs outbound test (client to server) . . . . . ", testDuration);
    fflush(stdout);

    // ....Fill buffer upto NDTConstants.PREDEFNED_BUFFER_SIZE packets
    pkts = 0;
    k = 0;
    for (i = 0; i < (64*KILO_BITS); i++) {  // again buffer sizes differ.
      // Since the actual transmitted byte count is timed, it doesn't appear
      // that it is causing specific problems.
      while (!isprint(k & 0x7f))
        k++;
      buff[i] = (k++ % 0x7f);
    }

    // set the test time to testDuration seconds
    t = secs();
    stop_time = t + testDuration;
    /* ignore the pipe signal */
    memset(&new, 0, sizeof(new));
    new.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &new, &old);

    for (i = 0; i < threadsnum; ++i) {
      writeWorkerArgs[i].connectionId = i + 1;
      writeWorkerArgs[i].stopTime = stop_time;
      writeWorkerArgs[i].buff = buff;
    }

    if (threadsnum == 1) {
      c2sWriteWorker((void*) &writeWorkerArgs[0]);
    }
    else {
      for (i = 0; i < threadsnum; ++i) {
        if (pthread_create(&writeWorkerIds[i], NULL, c2sWriteWorker, (void*) &writeWorkerArgs[i])) {
          log_println(0, "Cannot create write worker thread for throughput upload test!");
          writeWorkerIds[i] = 0;
          return -1;
        }
      }

      for (i = 0; i < threadsnum; ++i) {
        pthread_join(writeWorkerIds[i], NULL);
      }
    }
 
    sigaction(SIGPIPE, &old, NULL);
    sndqueue = sndq_len(writeWorkerArgs[0].socketDescriptor);  // get send-queue length

    // get actual duration for which data was sent to the server
    t = secs() - t;
    for (i = 0; i < threadsnum; ++i) {
      I2AddrFree(sec_addresses[i]);
    }
    I2AddrFree(sec_addr);

    // Calculate C2S throughput in kbps
    spdout = ((BITS_8_FLOAT * pkts * lth) / KILO) / t;
    log_println(6, " ---C->S CLT speed=%0.0f, pkts= %d, lth=%d, time=%0.0f", spdout, pkts, lth, t);


    // The client has stopped streaming data, and the server is now
    // expected to send a TEST_MSG message with the throughput it calculated.
    // So, its time now to receive this throughput (c2sspd).

    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed text message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_MSG, msgType, buff,
                       msgLen)) {
      // other message types at this juncture indicate error
      return 2;
    }
    if (msgLen <= 0) {  // message payload size cannot be negative! Error.
      log_println(0, "Improper message");
      return 3;
    }
    buff[msgLen] = 0;

        // get C->S test speed as calculated by server
    if (json_check_msg(buff) == 0) {
      jsonMsgValue = json_read_map_value(buff, DEFAULT_KEY);
      strlcpy(buff, jsonMsgValue, sizeof(buff));
      msgLen = strlen(buff);
      free(jsonMsgValue);
    }

    c2sspd = atoi(buff);
    if (throughputsnaps != NULL) {
      char* strtokptr = strtok(buff, " ");
      while ((strtokptr = strtok(NULL, " ")) != NULL) {
        if (lastThroughputSnapshot != NULL) {
          lastThroughputSnapshot->next = (struct throughputSnapshot*) malloc(sizeof(struct throughputSnapshot));
          lastThroughputSnapshot = lastThroughputSnapshot->next;
        }
        else {
          *uThroughputSnapshots = lastThroughputSnapshot = (struct throughputSnapshot*) malloc(sizeof(struct throughputSnapshot));
        }
        lastThroughputSnapshot->next = NULL;
        lastThroughputSnapshot->time = atof(strtokptr);
        strtokptr = strtok(NULL, " ");
        lastThroughputSnapshot->throughput = atof(strtokptr);
      }
    }

    // Print results in the most convenient units (kbps or Mbps)
    if (c2sspd < KILO)
      printf(" %0.2f kb/s\n", c2sspd);
    else
      printf(" %0.2f Mb/s\n", c2sspd/KILO);

    // Server should close test session with an empty TEST_FINALIZE message.
    // Any other type of message is an error.
    msgLen = sizeof(buff);
    if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
      log_println(0, "Protocol error - missed finalize message!");
      return 1;
    }
    if (check_msg_type("C2S throughput test", TEST_FINALIZE, msgType, buff,
                       msgLen)) {
      return 2;
    }
    log_println(1, " <------------------------->");
    // log protocol validation logs
    teststatuses = TEST_ENDED;
    protolog_status(getpid(), testids, teststatuses, ctlSocket);
    setCurrentTest(TEST_NONE);
  }
  return 0;
}

void* c2sWriteWorker(void* arg) {
  C2SWriteWorkerArgs *workerArgs = (C2SWriteWorkerArgs*) arg;
  int connectionId = workerArgs->connectionId;
  int socketDescriptor = workerArgs->socketDescriptor;
  double stopTime = workerArgs->stopTime;
  char* threadBuff = workerArgs->buff;
  double threadBytes = 0;
  int threadPackets = 0, n;
  double threadTime = secs();

  do {
    n = write(socketDescriptor, threadBuff, lth); // TODO avoid snd block
    // socket interrupted, continue attempting to write
    if ((n == -1) && (errno == EINTR))
      continue;
    if (n < 0)
      break;  // all data written. Exit
    threadPackets++;
    threadBytes += n;
  } while (secs() < stopTime);

  if (connectionId == 1) {
    pkts = threadPackets;
  }

  log_println(6, " ---C->S CLT thread %d (sc %d): speed=%0.0f, bytes=%0.0f, pkts= %d, lth=%d, time=%0.0f", connectionId, socketDescriptor,
                 ((BITS_8_FLOAT * threadBytes) / KILO) / (secs() - threadTime), threadBytes, threadPackets, lth, secs() - threadTime);

  return NULL;
}
