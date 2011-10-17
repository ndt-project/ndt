/*
 * This file contains the functions needed to handle META
 * test (client part).
 *
 * Jakub Sławiński 2011-05-07
 * jeremian@poczta.fm
 */

#include "../config.h"

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_meta.h"
#include "logging.h"
#include "network.h"
#include "protocol.h"
#include "utils.h"

int pkts, lth;
int sndqueue;
double spdout, c2sspd;

int test_meta_clt(int ctlSocket, char tests, char* host, int conn_options) {
	char buff[1024], tmpBuff[512];
	int msgLen, msgType;
	FILE * fp;

	if (tests & TEST_META) {
		log_println(1, " <-- META test -->");
		msgLen = sizeof(buff);
		if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
			log_println(0, "Protocol error - missed prepare message!");
			return 1;
		}
		if (check_msg_type("META test", TEST_PREPARE, msgType, buff, msgLen)) {
			return 2;
		}

		msgLen = sizeof(buff);
		if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
			log_println(0, "Protocol error - missed start message!");
			return 1;
		}
		if (check_msg_type("META test", TEST_START, msgType, buff, msgLen)) {
			return 2;
		}

		printf("sending meta information to server . . . . . ");
		fflush(stdout);

		if ((fp = fopen("/proc/sys/kernel/ostype", "r")) == NULL) {
			log_println(0, "Unable to determine client os type.");
		} else {
			fscanf(fp, "%s", tmpBuff);
			fclose(fp);
			sprintf(buff, "%s:%s", META_CLIENT_OS, tmpBuff);
			send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));
		}

		sprintf(buff, "%s:%s", META_BROWSER_OS, "- (web100clt)");
		send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));

		if ((fp = fopen("/proc/sys/kernel/osrelease", "r")) == NULL) {
			log_println(0, "Unable to determine client kernel version.");
		} else {
			fscanf(fp, "%s", tmpBuff);
			fclose(fp);
			sprintf(buff, "%s:%s", META_CLIENT_KERNEL_VERSION, tmpBuff);
			send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));
		}

		sprintf(buff, "%s:%s", META_CLIENT_VERSION, VERSION);
		send_msg(ctlSocket, TEST_MSG, buff, strlen(buff));

		send_msg(ctlSocket, TEST_MSG, "", 0);

		printf("Done\n");

		msgLen = sizeof(buff);
		if (recv_msg(ctlSocket, &msgType, buff, &msgLen)) {
			log_println(0, "Protocol error - missed finalize message!");
			return 1;
		}
		if (check_msg_type("META test", TEST_FINALIZE, msgType, buff, msgLen)) {
			return 2;
		}
		log_println(1, " <------------------------->");
	}
	return 0;
}
